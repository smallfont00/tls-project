
// HACK: This should be window.Terminal once upgraded to 4.0.1
var term = new Terminal();

term.open(document.getElementById('terminal'));

function XML_Send(path, data) {
    var xhr = new XMLHttpRequest();
    xhr.open("POST", path, true);

    //Send the proper header information along with the request
    xhr.setRequestHeader("Content-Type", "text/html");

    xhr.onreadystatechange = function () { // Call a function when the state changes.
        if (this.readyState === XMLHttpRequest.DONE && this.status === 200) {
            alert(xhr.responseText);
        }
    }
    xhr.send(data);
}

if (!!window.EventSource) {
    var source = new EventSource('/api/v1/pty')
    var ok = false
    source.addEventListener('message', function (e) {
        var data = window.atob(e.data)
        console.log(data);
        term.write(data);
    }, false)
    source.addEventListener('open', function (e) {
        alert("Connected");
    }, false)
    source.addEventListener('error', function (e) {
        if (e.eventPhase == EventSource.CLOSED)
            source.close()
        if (e.target.readyState == EventSource.CLOSED) {
            alert("Disconnected");
        }
        else if (e.target.readyState == EventSource.CONNECTING) {
            alert("Connecting...");
        }
    }, false)
    XML_Send('/api/v1/pty/write', '\n')
} else {
    console.log("Your browser doesn't support SSE")
}


function runFakeTerminal() {
    if (term._initialized) {
        return;
    }

    term._initialized = true;

    term.onData(e => {
        console.log(e);
        XML_Send('/api/v1/pty/write', e)
    })

}

runFakeTerminal();