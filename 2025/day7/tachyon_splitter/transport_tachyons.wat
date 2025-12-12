(module
    (import "js" "memory" (memory $memory 1))
    (export "transport_tachyons" (func $transport_tachyons))

    (func $transport_tachyons (param $input_offset i64) (param $input_size i64) (result i64)
        (local $grid_width i64)
        (local $start_tachyon_pos i64)
        (local $ch i32)

        i64.const 0
        local.tee $grid_width
        local.set $start_tachyon_pos
        (loop $scan_first_line
            ;; load character
            local.get $input_offset
            local.get $grid_width
            i64.add
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
            i32.const 0x0A ;; '\n'
            i32.ne
            (if ;; increment grid width and loop if not at a newline
                (then
                    local.get $grid_width
                    i64.const 1
                    i64.add
                    local.set $grid_width
                    br $scan_first_line
                )
            )
        )

        local.get $grid_width
    )
)
