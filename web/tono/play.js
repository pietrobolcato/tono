const emulator = new URL("https://joakimeriksson.github.io/fm1-emu/");
emulator.searchParams.set("package", new URL("tono.fwsc", location.href).href);
emulator.searchParams.set("name", "Tono");
emulator.searchParams.set("chip", "uc25hq80");
emulator.searchParams.set("mhz", "240");
document.querySelector("#standalone").href = emulator.href;
emulator.searchParams.set("embed", "1");
document.querySelector("#instrument").src = emulator.href;
