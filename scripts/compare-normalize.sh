# Sourced by compare-toolchains.sh. The menu demo logs the tick each
# scripted event was consumed at (sim_menu_demo.c) and documents it as
# visibility-only: which scheduler round consumes an event depends on
# lag under SIM. It is shown, not compared.
_fire_ticks_re='^([0-9A-F]{4} )+$'

normalize_trace() {
  if [ "$1" = epic-menu-demo ]; then
    sed -E "s/${_fire_ticks_re}/<fire ticks: shown, not compared>/" "$2"
  else
    cat "$2"
  fi
}

show_fire_ticks() {
  [ "$1" = epic-menu-demo ] || return 0
  echo "fire ticks (logged, not compared):"
  echo "  xc8:     $(grep -E "$_fire_ticks_re" "$2" || true)"
  echo "  epic-cc: $(grep -E "$_fire_ticks_re" "$3" || true)"
}
