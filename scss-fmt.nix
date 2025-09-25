{
  lib,
  stdenv,
}:
stdenv.mkDerivation {
  name = "scss-fmt";
  src = ./.;

  hardeningDisable = ["format"];

  env = {
    PREFIX = "${placeholder "out"}";
    RELEASE = true;
  };

  meta = {
    description = "Simple C SCSS Syntax formatter";
    maintainers = with lib.maintainers; [sigmanificient gurjaka];
    license = lib.licenses.bsd3;
    mainProgram = "scss-fmt";
  };
}
