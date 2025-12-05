let file_input = document.getElementById("input");
let result_box = document.getElementById("result");
let calculate_button = document.querySelector("button");

calculate_button.onclick = async function() {
    if (file_input.files.length == 0) {
        alert("no input file selected");
        return;
    }

    let input = new Uint8Array(await file_input.files[0].arrayBuffer());
    let total_joltage = 0;
    let bank_start = 0;
    while (bank_start < input.length) {
        // select first 12 batteries initially
        let selection_end = bank_start + 12;
        let selection = input.slice(bank_start, selection_end);
        bank_start = selection_end;

        while (true) {
            let battery = input[bank_start++];
            if (battery == 0x0D) continue; // skip carriage returns
            if (battery == 0x0A) break; // newline delimits banks

            let found_battery_to_promote = false;
            for (let i = 1; i < selection.length; i++) {
                if (selection[i] > selection[i - 1]) {
                    // promote this battery and shift successive batteries back
                    for (let j = i; j < selection.length; j++) {
                        selection[j - 1] = selection[j];
                    }

                    found_battery_to_promote = true;
                    break;
                }
            }

            if (found_battery_to_promote || battery > selection[selection.length - 1]) {
                // either last slot was emptied or needs to be replaced
                selection[selection.length - 1] = battery;
            }
        }

        let max_joltage_for_bank = 0;
        for (let digit of selection) {
            digit -= 0x30; // offset from ASCII '0'
            max_joltage_for_bank = max_joltage_for_bank * 10 + digit;
        }

        total_joltage += max_joltage_for_bank;
    }

    result_box.innerText = total_joltage.toString();
};
