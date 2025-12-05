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
            let ch = input[bank_start++];
            if (ch == 0x0D) continue; // skip carriage returns
            if (ch == 0x0A) break; // newline delimits banks

            //
        }
    }
};