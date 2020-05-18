
// HACK: This should be window.Terminal once upgraded to 4.0.1
var term = new Terminal();

term.open(document.getElementById('terminal'));

var xhr = new XMLHttpRequest();
xhr.open("POST", '/api/v1/pty', true);

//Send the proper header information along with the request
xhr.setRequestHeader("Content-Type", "text/html");

xhr.onreadystatechange = function () { // Call a function when the state changes.
    if (this.readyState === XMLHttpRequest.DONE && this.status === 200) {
        alert(xhr.responseText);
    }
}
xhr.send("start");

if (!!window.EventSource) {
    var source = new EventSource('/api/v1/pty')
    var ok = false
    source.addEventListener('message', function (e) {
        console.log(e.data);
        term.write(e.data);
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

    var xhr = new XMLHttpRequest();
    xhr.open("POST", '/api/v1/pty/write', true);

    //Send the proper header information along with the request
    xhr.setRequestHeader("Content-Type", "text/html");

    xhr.onreadystatechange = function () { // Call a function when the state changes.
        if (this.readyState === XMLHttpRequest.DONE && this.status === 200) {
            //alert(xhr.responseText);
        }
    }
    xhr.send("\n");
} else {
    console.log("Your browser doesn't support SSE")
}


function runFakeTerminal() {
    if (term._initialized) {
        return;
    }

    term._initialized = true;

    term.prompt = () => {
        term.write('\r\n$ ');
    };

    term.writeln('Welcome to xterm.js');
    term.writeln('This is a local terminal emulation, without a real terminal in the back-end.');
    term.writeln('Type some keys and commands to play around.');
    term.writeln('');


    prompt(term);

    term.onData(e => {
        var xhr = new XMLHttpRequest();
        xhr.open("POST", '/api/v1/pty/write', true);

        //Send the proper header information along with the request
        xhr.setRequestHeader("Content-Type", "text/html");

        xhr.onreadystatechange = function () { // Call a function when the state changes.
            if (this.readyState === XMLHttpRequest.DONE && this.status === 200) {
                //alert(xhr.responseText);
            }
        }
        xhr.send(e);
    })
    /*
        term.onKey(e => {
            const printable = !e.domEvent.altKey && !e.domEvent.altGraphKey && !e.domEvent.ctrlKey && !e.domEvent.metaKey;
    
    
            if (e.domEvent.keyCode === 13) {
                prompt(term);
            } else if (e.domEvent.keyCode === 8) {
                // Do not delete the prompt
                if (term._core.buffer.x > 2) {
                    term.write('\b \b');
                }
            } else if (printable) {
                term.write(e.key);
            }
        });
    */
}

function prompt(term) {
    term.write('\r\n$ ');
}
runFakeTerminal();