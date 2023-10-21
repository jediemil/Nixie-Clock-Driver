function sendCatCommand() {
    let tube0 = document.getElementById("tube0").checked
    let tube1 = document.getElementById("tube1").checked
    let tube2 = document.getElementById("tube2").checked
    let tube3 = document.getElementById("tube3").checked

    const xhr = new XMLHttpRequest();
    xhr.open("POST", "/cathode_pois");
    xhr.setRequestHeader("Content-Type", "application/json; charset=UTF-8")
    if (tube0) xhr.setRequestHeader("tube0", "1")
    if (tube1) xhr.setRequestHeader("tube1", "1")
    if (tube2) xhr.setRequestHeader("tube2", "1")
    if (tube3) xhr.setRequestHeader("tube3", "1")
    xhr.send();
}