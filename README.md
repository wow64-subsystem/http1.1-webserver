# http1.1-webserver
 HTTP/1.1 Simple Web Server

 Simple HTTP/1.1 web server that can serve any content, it's barebones, and the only implementation currently is: 'Content-Length,' and 'Content-Type.' If it receive an HTTP protocol that is not HTTP/1.1 it will send an error, and if it receives any method besides 'GET'. It will send an error 'Not Implemented.'
 