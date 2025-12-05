let file_input = document.getElementById("input");
let result_box = document.getElementById("result");
let calculate_button = document.querySelector("button");

calculate_button.onclick = async function() {
    if (file_input.files.length == 0) {
        alert("no input file selected");
        return;
    }

    let input = await file_input.files[0].text();
    //
};