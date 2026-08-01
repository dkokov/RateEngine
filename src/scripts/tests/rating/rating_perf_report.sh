#!/usr/bin/env bash
#
# rating_perf_report.sh - offline-rating throughput baseline from a RateEngine log.
#
# The rating loop logs one line per batch (rating.c, needs LogDebugLevel >= INFO):
#     rating_loop() ... batch times: <total_s> total, <avg_s> avg/cdr, cdrs: <n>
# where total_s is wall seconds for the batch (gettimeofday) and avg_s = total_s/n.
# This parses those lines and reports:
#   * overall throughput  (cdr/s and ms/cdr across all batches)
#   * per-batch ms/cdr     min / median / max
#   * degradation trend    (mean ms/cdr of the first 25% of batches vs the last
#                           25%) - the "0.45ms -> 4.9ms over an hour" effect the
#                           single-mutex + table-bloat issues used to cause.
#
# It does NOT touch the DB or run anything - it reads a log you already produced
# by rating a representative workload with rt.so. Capture a baseline BEFORE the
# lock/atomic-balance changes, then re-run after and compare the same numbers.
#
# usage: rating_perf_report.sh [logfile] [label]
#   logfile : default /usr/local/RateEngine/logs/rate_engine.log
#   label   : if given, append a one-line summary to baseline_results.tsv so
#             before/after runs accumulate in one place (e.g. "0.7.6-baseline").
set -u

HERE=$(cd "$(dirname "$0")" && pwd)
LOG=${1:-/usr/local/RateEngine/logs/rate_engine.log}
LABEL=${2:-}
BASELINE_TSV="$HERE/baseline_results.tsv"

[ -f "$LOG" ] || { echo "rating_perf_report: log not found: $LOG" >&2; exit 2; }

# Pull "<total> <avg> <cdrs>" from every batch-times line (ignores the LOG
# timestamp/prefix). avg is seconds/cdr; we report ms/cdr = avg*1000.
mapfile -t ROWS < <(grep -oE 'batch times: [0-9.]+ total, [0-9.]+ avg/cdr, cdrs: [0-9]+' "$LOG" \
	| sed -E 's/batch times: ([0-9.]+) total, ([0-9.]+) avg\/cdr, cdrs: ([0-9]+)/\1 \2 \3/')

N=${#ROWS[@]}
if [ "$N" -eq 0 ]; then
	echo "rating_perf_report: no 'batch times:' lines in $LOG"
	echo "  (rate a workload with rt.so at LogDebugLevel >= INFO first)"
	exit 1
fi

printf '%s\n' "${ROWS[@]}" | awk -v n="$N" -v label="$LABEL" -v tsv="$BASELINE_TSV" -v logf="$LOG" '
{
	total += $1; cdrs += $3;
	ms = $2 * 1000.0;          # avg s/cdr -> ms/cdr
	msv[NR] = ms;
	batch_total[NR] = $1; batch_cdrs[NR] = $3;
}
END {
	# overall
	overall_mscdr = (cdrs > 0) ? (total * 1000.0 / cdrs) : 0;
	cdr_s         = (total > 0) ? (cdrs / total) : 0;

	# sort per-batch ms/cdr for min/median/max
	for (i = 1; i <= n; i++) sorted[i] = msv[i];
	for (i = 1; i <= n; i++) for (j = i+1; j <= n; j++)
		if (sorted[j] < sorted[i]) { t = sorted[i]; sorted[i] = sorted[j]; sorted[j] = t; }
	mn = sorted[1]; mx = sorted[n];
	med = (n % 2) ? sorted[(n+1)/2] : (sorted[n/2] + sorted[n/2+1]) / 2.0;

	# degradation: mean ms/cdr of first 25% of batches vs last 25% (in log order)
	q = int(n/4); if (q < 1) q = 1;
	for (i = 1; i <= q; i++)       early += msv[i];
	for (i = n-q+1; i <= n; i++)   late  += msv[i];
	early /= q; late /= q;
	degr = (early > 0) ? (late / early) : 0;

	printf "\n==================== rating perf baseline ====================\n";
	printf "  log            : %s\n", logf;
	printf "  batches        : %d   (total CDRs rated: %d)\n", n, cdrs;
	printf "  wall (sum)     : %.2f s\n", total;
	printf "  overall        : %.3f ms/cdr   (%.1f cdr/s)\n", overall_mscdr, cdr_s;
	printf "  per-batch ms/cdr: min %.3f  median %.3f  max %.3f\n", mn, med, mx;
	printf "  degradation    : early %.3f ms/cdr -> late %.3f ms/cdr  (%.2fx)\n", early, late, degr;
	if (degr >= 1.5) printf "  >> WARNING: late batches %.2fx slower than early - runtime degradation present\n", degr;
	printf "==============================================================\n";

	if (label != "") {
		# TSV: label  batches  cdrs  wall_s  overall_mscdr  cdr_s  min  median  max  degr_x
		printf "%s\t%d\t%d\t%.2f\t%.3f\t%.1f\t%.3f\t%.3f\t%.3f\t%.2f\n",
			label, n, cdrs, total, overall_mscdr, cdr_s, mn, med, mx, degr >> tsv;
		printf "  recorded as \"%s\" in %s\n", label, tsv;
	}
}'

# Ensure the TSV has a header the first time a label is recorded.
if [ -n "$LABEL" ] && [ -f "$BASELINE_TSV" ]; then
	if ! head -1 "$BASELINE_TSV" | grep -q '^label'; then
		sed -i '1i label\tbatches\tcdrs\twall_s\toverall_ms_cdr\tcdr_s\tmin_ms\tmedian_ms\tmax_ms\tdegr_x' "$BASELINE_TSV"
	fi
fi
