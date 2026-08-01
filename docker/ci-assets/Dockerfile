# Generic blob carrier, not a runnable image. Embeds whatever file(s) sit
# in the build context (a vendor installer we're not allowed to publicly
# redistribute, see docs/ci-plan.md "Redistribution, not just download
# blocking" for why) as a private GHCR image. Consumers extract the file
# with `docker create` + `docker cp`, never `docker run` (there is nothing
# to run, FROM scratch has no OS, no shell). CMD is a placeholder so
# `docker create` has something to record; it is never executed.
FROM scratch
COPY . /
CMD ["/nonexistent"]
