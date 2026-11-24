"""Stage the Tono page with a pinned fm1-emu checkout and its built WASM."""
import json
from pathlib import Path
import shutil
import sys

engine, wasm, site = map(Path, sys.argv[1:4])
version = sys.argv[4]
web = engine / "web"
here = Path(__file__).resolve().parent
for name in ("style.css", "main.js", "persistence.js", "tryout.js", "worklet.js", "jit.js", "fwsc.js"):
    shutil.copy2(web / name, site / name)
shutil.copy2(wasm, site / "fm1_web.wasm")
shutil.copy2(engine / "LICENSE", site / "EMULATOR_LICENSE")
shutil.copytree(web / "fonts", site / "fonts", dirs_exist_ok=True)
(site / "smk37").mkdir(exist_ok=True)
(site / "smk37/firmwares.json").write_text('{"firmwares": []}\n')
(site / "shots").mkdir(exist_ok=True)
(site / "shots/shots.json").write_text("{}\n")
shutil.copy2(here / "play.css", site / "play.css")
upstream_page = (web / "index.html").read_text()
player = upstream_page[upstream_page.index('  <div class="player" id="play">'):
                       upstream_page.index("  <!-- what's new in this release")]
player = player.replace('href="#guide"', 'href="https://github.com/pietrobolcato/tono/blob/main/ambplus/CONTROLS.md"')
(site / "index.html").write_text((here / "index.html").read_text().replace("      <!-- FM1_PLAYER -->", player))
# Tono has one release-specific catalog entry. Its ID keeps older browser saves
# available while fresh sessions pick the current build.
identifier = "tono-" + version
(site / "firmwares.json").write_text(json.dumps({"default": identifier, "firmwares": [{
    "id": identifier, "name": "Tono", "version": version, "author": "pietrobolcato",
    "license": "GPL-3.0-only", "about": "Generative ambient firmware.",
    "repo": "https://github.com/pietrobolcato/tono", "package": "tono.fwsc"
}]}))
main = site / "main.js"
script = main.read_text()
start = script.index("let hubWait;")
end = script.index("const byId", start)
script = script[:start] + "const newcomers = [];\n" + script[end:]
script = script.replace('stored("fm1.mhz")) ?? 100', 'stored("fm1.mhz")) ?? 240')
script = script.replace('? "uc25hq80" : "p25q80";', '? "p25q80" : "uc25hq80";').replace('["uc25hq80", "new"].includes','["p25q80"].includes')
main.write_text(script)
