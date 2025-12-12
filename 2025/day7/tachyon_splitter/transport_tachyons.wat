(module
    (import "js" "memory" (memory $memory 1))
    (export "transport_tachyons" (func $transport_tachyons))

    (func $transport_tachyons (param $input_offset i32) (param $input_size i32) (result i32)
        (local $ch i32)
        (local $grid_width i32)
        (local $line_stride i32)
        (local $line_offset i32)
        (local $input_end_offset i32)
        (local $start_tachyon_pos i32)
        (local $tachyons_in_offset i32)
        (local $tachyons_out_offset i32)
        (local $tachyon_array_size i32)
        (local $required_extra_pages i32)
        (local $tachyons_in_count i32)
        (local $tachyons_out_count i32)
        (local $tachyons_end_offset i32)
        (local $tachyon_pos_offset i32)

        local.get $input_offset
        local.get $input_size
        i32.add
        local.set $input_end_offset

        i32.const 0
        local.tee $grid_width
        local.set $start_tachyon_pos
        (loop $scan_first_line
            ;; load character
            local.get $input_offset
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
        i32.const 3
        i32.shl
        local.set $tachyon_array_size
        ;; tachyon_array_size = grid_width * sizeof(u64)

        local.get $input_offset
        local.get $input_size
        i32.add
        i32.const 7
        i32.add
        i32.const 0xFFFFFFF8
        i32.and
        local.tee $tachyons_in_offset
        ;; tachyons_in_offset = (input_offset + input_size + 7) / 8 * 8
        ;; = round_up(input_offset + input_size, 8)

        local.get $tachyon_array_size
        i32.add
        local.tee $tachyons_out_offset

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

        local.get $input_offset
        local.get $line_stride
        i32.add
        local.set $line_offset
        ;; start propagating to the second line

        i32.const 1
        local.set $tachyons_in_count
        local.get $tachyons_in_offset
        local.get $start_tachyon_pos
        i32.store
        ;; initialize incoming tachyons array with the starting tachyon

        (loop $trickle_down
            local.get $line_offset
            local.get $input_end_offset
            i32.lt_u
            (if
                (then
                    local.get $tachyons_in_offset
                    local.tee $tachyon_pos_offset
                    local.get $tachyon_array_size
                    i32.add
                    local.set $tachyons_end_offset
                    (loop $propagate_tachyons
                        local.get $tachyon_pos_offset
                        local.get $tachyons_end_offset
                        i32.lt_u
                        (if
                            (then
                                local.get $tachyon_pos_offset
                                i32.load
                                ;; ^ tachyon position
                                drop

                                local.get $tachyon_pos_offset
                                i32.const 4
                                i32.add
                                local.set $tachyon_pos_offset
                                ;; increment to offset of next incoming tachyon position

                                br $propagate_tachyons
                            )
                        )
                    )

                    local.get $tachyons_in_offset
                    local.get $tachyons_out_offset
                    local.set $tachyons_in_offset
                    local.set $tachyons_out_offset
                    local.get $tachyons_out_count
                    local.set $tachyons_in_count
                    ;; swapped incoming tachyons array with outgoing tachyons array

                    local.get $line_offset
                    local.get $line_stride
                    i32.add
                    local.set $line_offset
                    ;; incremented to next line

                    br $trickle_down
                )
            )
        )

        local.get $tachyons_in_offset
    )
)
