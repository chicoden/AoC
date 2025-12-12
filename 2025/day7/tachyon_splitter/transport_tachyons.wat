(module
    (import "js" "memory" (memory $memory 1))
    (export "transport_tachyons" (func $transport_tachyons))

    (func $transport_tachyons (param $input_offset i32) (param $input_size i32) (result i32)
        (local $grid_width i32)
        (local $start_tachyon_pos i32)
        (local $ch i32)
        (local $line_stride i32)

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

        local.get $line_stride
        local.get $grid_width
        i32.sub
    )
)
