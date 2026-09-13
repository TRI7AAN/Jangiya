#!/bin/sh
# Phase 7.5 fixture: poison script for untaken branches (exit 7).
# If this ever executes, the run visibly fails — proving a branch that
# must stay dead actually stayed dead is then airtight.
# @jocky:function jky_recon_false_branch
# @jocky:domain recon
# @jocky:description Fixture poison script that must never execute
# @jocky:inputs target: string = "default"
# @jocky:outputs result: table<node>
# @jocky:capability recon.fixture.execute
# @jocky:timeout_seconds 30
# @jocky:depends_on
echo "branch-false-poison"
exit 7
