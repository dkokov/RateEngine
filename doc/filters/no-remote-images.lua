-- Fix-ups applied when the separate .md files are merged into one document.

-- 1. Drop images that pandoc cannot embed in a PDF:
--      * remote images (http/https) - e.g. the shields.io license badge
--      * SVG images, which need rsvg-convert + the LaTeX svg.sty package
--    The caption/link text (if any) is kept, so nothing is silently lost.
function Image(img)
  local src = img.src or ""
  if src:match("^https?://") or src:match("%.svg$") then
    return img.caption or {}
  end
  return nil
end

-- 2. Repair internal cross references.
--    The docs use hand-written anchors like "#CalcFunctions", but pandoc
--    generates lowercase gfm identifiers ("calcfunctions"), and links to
--    another file ("config.md#Foo" / "config.md") do not exist once every
--    file is one chapter of the same document.
function Link(link)
  local tgt = link.target or ""
  if tgt:match("^%a+://") or tgt:match("^mailto:") then
    return nil                                   -- external, leave alone
  end

  local anchor = tgt:match("#(.*)$")
  local file   = tgt:match("^([^#]*)")

  if anchor and anchor ~= "" then
    link.target = "#" .. anchor:lower()
    return link
  end

  -- Link to a whole .md file -> point at that file's top-level heading.
  if file and file:match("%.md$") then
    return link.content                          -- no reliable target: plain text
  end

  return nil
end
