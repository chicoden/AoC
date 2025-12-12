(async function main() {
    let memory = new WebAssembly.Memory({ initial: 1 });
    let module = await WebAssembly.instantiateStreaming(fetch("transport_tachyons.wasm"), {
        js: { memory }
    });

    let file_input = document.getElementById("input");
    let problem_box = document.getElementById("problem");
    let result_box = document.getElementById("result");
    let calculate_button = document.querySelector("button");
    let input_file = null;

    file_input.onchange = async function() {
        try {
            input_file = new Uint8Array(await file_input.files[0].arrayBuffer());
            problem_box.innerText = String.fromCharCode(...input_file);
        } catch (error) {
            input_file = null;
            problem_box.innerText = "";
        }
    };

    calculate_button.onclick = async function() {
        if (input_file === null) {
            alert("no input file ready");
            return;
        }

        let input_offset = 0;
        let input_size = input_file.byteLength;
        new Uint8Array(memory.buffer, input_offset, input_size).set(input_file);
        let split_count = module.instance.exports.transport_tachyons(input_offset, input_size);
        problem_box.innerText = String.fromCharCode(...new Uint8Array(memory.buffer, input_offset, input_size));
        result_box.innerText = split_count.toString();
    };
})();
