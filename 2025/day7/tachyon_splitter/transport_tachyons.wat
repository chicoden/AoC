(module
    (import "js" "memory" (memory $memory 1))
    (export "transport_tachyons" (func $transport_tachyons))

    (func $transport_tachyons (param $input_start i32) (param $input_size i32) (result i32)
        (local $ch i32)
        (local $grid_width i32)
        (local $line_stride i32)
        (local $line_start i32)
        (local $input_end i32)
        (local $start_tachyon_pos i32)
        (local $tachyons_in_start i32)
        (local $tachyons_out_start i32)
        (local $tachyons_in_end i32)
        (local $tachyons_out_end i32)
        (local $tachyon_array_size i32)
        (local $required_extra_pages i32)
        (local $tachyon_pos_offset i32)
        (local $tachyon_pos i32)
        (local $propagation_offset i32)
        (local $propagation_target i32)
        (local $split_pos i32)
        (local $split_offset i32)
        (local $split_count i32)

        local.get $input_start
        local.get $input_size
        i32.add
        local.set $input_end

        i32.const 0
        local.tee $grid_width
        local.set $start_tachyon_pos
        (loop $scan_first_line
            ;; load character
            local.get $input_start
            local.get $grid_width
            i32.add
            i32.load8_u

            local.tee $ch
            i32.const 0x53 ;; 'S'
            i32.eq
            (if
                (then
                    local.get $grid_width
                    local.set $start_tachyon_pos
                )
            )

            local.get $ch
            i32.const 0x0D ;; '\r'
            i32.ne
            local.get $ch
            i32.const 0x0A ;; '\n'
            i32.ne
            i32.and
            (if ;; increment grid width and loop if not at a carriage return or a newline
                (then
                    local.get $grid_width
                    i32.const 1
                    i32.add
                    local.set $grid_width
                    br $scan_first_line
                )
            )
        )

        i32.const 2 ;; if stopped at '\r', expect to skip '\r\n' at end of each line
        i32.const 1 ;; if stopped at '\n', expect to skip '\n' at end of each line
        local.get $ch
        i32.const 0x0D ;; '\r'
        i32.eq
        select
        local.get $grid_width
        i32.add
        local.set $line_stride

        local.get $grid_width
        i32.const 2
        i32.shl
        local.set $tachyon_array_size
        ;; tachyon_array_size = grid_width * sizeof(u32)

        local.get $input_end
        i32.const 3
        i32.add
        i32.const 0xFFFFFFFC
        i32.and
        local.tee $tachyons_in_start
        ;; tachyons_in_start = round_up(input_end, alignof(u32))

        local.get $tachyon_array_size
        i32.add
        local.tee $tachyons_out_start

        local.get $tachyon_array_size
        i32.add
        ;; ^ required size in bytes
        i32.const 65535
        i32.add
        i32.const 16
        i32.shr_u
        ;; ^ ceil(required_size / 64KiB)
        memory.size $memory
        i32.sub
        local.tee $required_extra_pages
        i32.const 0
        i32.gt_s
        (if ;; required_extra_pages > 0
            (then
                local.get $required_extra_pages
                memory.grow $memory
                i32.const 0
                i32.lt_s
                (if ;; memory.grow returned negative (an error)
                    (then
                        i32.const -1
                        return
                    )
                )
            )
        )

        local.get $input_start
        local.get $line_stride
        i32.add
        local.set $line_start
        ;; start propagating to the second line

        local.get $tachyons_in_start
        local.get $start_tachyon_pos
        i32.store
        local.get $tachyons_in_start
        i32.const 4 ;; sizeof(u32)
        i32.add
        local.set $tachyons_in_end
        ;; initialize incoming tachyons array with the starting tachyon

        i32.const 0
        local.set $split_count
        (loop $trickle_down
            local.get $line_start
            local.get $input_end
            i32.lt_u
            (if
                (then
                    local.get $tachyons_out_start
                    local.set $tachyons_out_end
                    ;; array of outgoing tachyons is initially empty

                    local.get $tachyons_in_start
                    local.set $tachyon_pos_offset
                    ;; start with first incoming tachyon

                    (loop $propagate_tachyons
                        local.get $tachyon_pos_offset
                        local.get $tachyons_in_end
                        i32.lt_u
                        (if
                            (then
                                local.get $tachyon_pos_offset
                                i32.load
                                local.tee $tachyon_pos
                                local.get $line_start
                                i32.add
                                local.tee $propagation_offset
                                i32.load8_u

                                local.tee $propagation_target
                                i32.const 0x2E ;; '.'
                                i32.eq
                                (if
                                    (then ;; propagating into free space
                                        local.get $propagation_offset
                                        i32.const 0x7C ;; '|'
                                        i32.store8
                                        ;; marked where the beam propagated

                                        local.get $tachyons_out_end
                                        local.get $tachyon_pos
                                        i32.store
                                        local.get $tachyons_out_end
                                        i32.const 4 ;; sizeof(u32)
                                        i32.add
                                        local.set $tachyons_out_end
                                        ;; added tachyon to the outgoing tachyons
                                    )
                                    (else
                                        local.get $propagation_target
                                        i32.const 0x5E ;; '^'
                                        i32.eq
                                        (if ;; splitting
                                            (then
                                                local.get $split_count
                                                i32.const 1
                                                i32.add
                                                local.set $split_count
                                                ;; counted the split

                                                local.get $tachyon_pos
                                                i32.const 1
                                                i32.sub
                                                local.tee $split_pos
                                                i32.const 0
                                                i32.ge_s
                                                (if ;; can propagate left
                                                    (then
                                                        local.get $line_start
                                                        local.get $split_pos
                                                        i32.add
                                                        local.tee $split_offset
                                                        i32.load8_u
                                                        i32.const 0x2E ;; '.'
                                                        i32.eq
                                                        (if ;; spot has not been taken
                                                            (then
                                                                local.get $split_offset
                                                                i32.const 0x7C ;; '|'
                                                                i32.store8
                                                                ;; marked where the beam propagated

                                                                local.get $tachyons_out_end
                                                                local.get $split_pos
                                                                i32.store
                                                                local.get $tachyons_out_end
                                                                i32.const 4 ;; sizeof(u32)
                                                                i32.add
                                                                local.set $tachyons_out_end
                                                                ;; added tachyon to the outgoing tachyons
                                                            )
                                                        )
                                                    )
                                                )

                                                local.get $tachyon_pos
                                                i32.const 1
                                                i32.add
                                                local.tee $split_pos
                                                local.get $grid_width
                                                i32.lt_s
                                                (if ;; can propagate right
                                                    (then
                                                        local.get $line_start
                                                        local.get $split_pos
                                                        i32.add
                                                        local.tee $split_offset
                                                        i32.load8_u
                                                        i32.const 0x2E ;; '.'
                                                        i32.eq
                                                        (if ;; spot has not been taken
                                                            (then
                                                                local.get $split_offset
                                                                i32.const 0x7C ;; '|'
                                                                i32.store8
                                                                ;; marked where the beam propagated

                                                                local.get $tachyons_out_end
                                                                local.get $split_pos
                                                                i32.store
                                                                local.get $tachyons_out_end
                                                                i32.const 4 ;; sizeof(u32)
                                                                i32.add
                                                                local.set $tachyons_out_end
                                                                ;; added tachyon to the outgoing tachyons
                                                            )
                                                        )
                                                    )
                                                )
                                            )
                                        )
                                    )
                                )

                                local.get $tachyon_pos_offset
                                i32.const 4 ;; sizeof(u32)
                                i32.add
                                local.set $tachyon_pos_offset
                                br $propagate_tachyons
                            )
                        )
                    )

                    local.get $tachyons_in_start
                    local.get $tachyons_out_start
                    local.set $tachyons_in_start
                    local.set $tachyons_out_start
                    local.get $tachyons_out_end
                    local.set $tachyons_in_end
                    ;; swapped allocations for incoming and outgoing tachyons
                    ;; the outgoing tachyons become the incoming tachyons

                    local.get $line_start
                    local.get $line_stride
                    i32.add
                    local.set $line_start
                    br $trickle_down
                )
            )
        )

        local.get $split_count
        ;; leave split count on the stack as the return value
    )
)
