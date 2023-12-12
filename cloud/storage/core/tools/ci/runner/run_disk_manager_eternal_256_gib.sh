#!/usr/bin/env bash
export instance_cores=16
export instance_ram=16

export disk_size_gib=256
export disk_block_size=4096
export disk_write_size_percentage=5
export test_suite="eternal_256gib"

source "${0%/*}/disk_manager_acceptance_common.sh"
run_eternal
