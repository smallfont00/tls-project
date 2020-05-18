
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

XML_Send('/api/v1/pty', "start")

var reg = /\[00m\$/

if (!!window.EventSource) {
    var source = new EventSource('/api/v1/pty')
    var ok = false
    source.addEventListener('message', function (e) {
        var data = window.atob(e.data)
        console.log(data);
        //if (reg.test(data)) {
        //    console.log('last message');
        //    data = '\r\n' + data;
        //}
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

    // prompt(term);
    var buffer = ""

    term.onData(e => {
        buffer += e;
        console.log(buffer);
        XML_Send('/api/v1/pty/write', buffer)
        buffer = ""
    })



    term.onKey(e => {
        const printable = !e.domEvent.altKey && !e.domEvent.altGraphKey && !e.domEvent.ctrlKey && !e.domEvent.metaKey;


        if (e.domEvent.keyCode === 13) {

            //term.write('\r\n')
            //XML_Send('/api/v1/pty/write', '\n')
            //buffer = ""
            //prompt(term);
        } else if (e.domEvent.keyCode === 8) {
            // Do not delete the prompt
            //term.write('\b \b');
        } else if (printable) {
            //term.write(e.key);
        }
    });
}

// function prompt(term) {
//     term.write('\r\n$ ');
// }
runFakeTerminal();