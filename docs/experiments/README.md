# Experiments

Each subdirectory is a self-contained experiment: its own README (the
record: question, method, results, conclusion) and the tools used to run
it. Experiments are probes, not product code; they document facts the
repo relies on, in the same spirit as layout-budgets.md.

- [math-cycle-benchmark](math-cycle-benchmark/README.md): epic-math hand
  asm vs XC8 native arithmetic across -O0..-O3 (2026-08-12). Includes
  the follow-up study of the -O3 library techniques and why they do not
  transfer to hand-asm.
