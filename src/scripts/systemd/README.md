# RateEngine systemd unit

`rate-engine.service` is the systemd replacement for the legacy
`scripts/init.d/RateEngine` SysV script.

RateEngine daemonizes itself (`-d`) and manages its own PID file, so the unit
uses `Type=forking` with an explicit `PIDFile=`.

## Install

```
# 1. copy the unit into place
sudo cp rate-engine.service /etc/systemd/system/

# 2. reload systemd so it sees the new unit
sudo systemctl daemon-reload

# 3. start now and enable on boot
sudo systemctl enable --now rate-engine.service
```

## Manage

```
sudo systemctl start   rate-engine
sudo systemctl stop    rate-engine
sudo systemctl restart rate-engine
sudo systemctl status  rate-engine
```

## Before you start - check these match your install

* **Binary / config paths** in `ExecStart` / `ExecStop`. Defaults assume the
  standard install (`/usr/local/RateEngine/`) and the `RateEngine7.xml` sample
  config. Override the config path with the `RE_CONF=` line in the unit, or with
  a drop-in (`systemctl edit rate-engine`).

* **`PIDFile=`** must match the `PIDFile` value in your RateEngine XML,
  resolved against `DIR`. With the default config that is:

  ```
  DIR      = /usr/local/RateEngine/
  PIDFile  = logs/rate_engine.pid
  -> /usr/local/RateEngine/logs/rate_engine.pid
  ```

  If they don't match, systemd cannot track the daemon and `stop`/`status`
  will misbehave.

## Notes

* The daemon redirects its own stdout/stderr to `/dev/null` and writes to its
  log file (`LogFile` in the XML, default `logs/rate_engine.log`), so
  `journalctl -u rate-engine` will be mostly empty - tail the log file instead:

  ```
  tail -f /usr/local/RateEngine/logs/rate_engine.log
  ```

* RateEngine refuses to start a second instance while a live PID file exists
  (single-instance guard), so `restart` is safe.

* To run as a non-root user, create the user/group, give it ownership of
  `/usr/local/RateEngine/logs/`, and uncomment the `User=`/`Group=` lines in
  the unit. Running as root is only needed for raw sockets / call-control.
