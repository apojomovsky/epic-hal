# Sourced by compare-toolchains.sh. The menu and control demos log the
# tick each scripted event was consumed at (sim_menu_demo.c,
# sim_control_demo.c) and document it as visibility-only: which
# scheduler round consumes an event depends on lag under SIM. It is
# shown, not compared. Bridge defines the same emitter but never calls
# it (epic-hal#345), so its id-scoped rule is a no-op until wired.
_fire_ticks_re='^([0-9A-F]{4} )+$'

is_fire_tick_module() {
  case "$1" in
    epic-menu-demo|epic-control-demo|epic-bridge-demo) return 0;;
    *) return 1;;
  esac
}

normalize_trace() {
  if is_fire_tick_module "$1"; then
    sed -E "s/${_fire_ticks_re}/<fire ticks: shown, not compared>/" "$2"
  else
    cat "$2"
  fi
}

show_fire_ticks() {
  is_fire_tick_module "$1" || return 0
  echo "fire ticks (logged, not compared):"
  echo "  xc8:     $(grep -E "$_fire_ticks_re" "$2" || true)"
  echo "  epic-cc: $(grep -E "$_fire_ticks_re" "$3" || true)"
}
