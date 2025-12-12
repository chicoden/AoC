(async function main() {
    let memory = new WebAssembly.Memory({ initial: 1 });
    let module = await WebAssembly.instantiateStreaming(fetch("transport_tachyons.wasm"), {
        js: { memory, log: x => console.log(`Value: ${x}`) }
    });

    let file_input = document.getElementById("input");
    let problem_box = document.getElementById("problem");
    let result_box = document.getElementById("result");
    let calculate_button = document.querySelector("button");
    let input_data = null;

    file_input.onchange = async function() {
        input_data = new Uint8Array(await file_input.files[0].arrayBuffer());
        problem_box.innerText = String.fromCharCode(...input_data);
    };

    calculate_button.onclick = async function() {
        if (input_data === null) {
            alert("no input file ready");
            return;
        }

        let input_size = input_data.byteLength;
        let input_offset = 0;

        console.log(memory.buffer.byteLength);
        console.log(input_data);
        new Uint8Array(memory.buffer, input_offset, input_size).set(input_data);
        let result = module.instance.exports.transport_tachyons(input_offset, input_size);
        console.log(String.fromCharCode(...new Uint8Array(memory.buffer, input_offset, input_size)));
        console.log(result);
        console.log(memory.buffer.byteLength);
        console.log(memory.buffer);
    };
})();
