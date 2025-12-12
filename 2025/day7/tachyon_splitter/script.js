(async function main() {
    let memory = new WebAssembly.Memory({ initial: 1 });
    let module = await WebAssembly.instantiateStreaming(fetch("transport_tachyons.wasm"), {
        js: { memory }
    });

    let input_data = new Uint8Array([..."..S.\n..^.\n....\n.^..\n....\n"].map(ch => ch.charCodeAt(0)));
    new Uint8Array(memory.buffer).set(input_data);
    let result = module.instance.exports.transport_tachyons(0, input_data.byteLength);
    console.log(String.fromCharCode(...new Uint8Array(memory.buffer.slice(0, input_data.byteLength))));
    console.log(result);
})();
