# proxy

Current features:
- Forwarding simple HTTP requests

### Installation
1. create `proxy.db` file 
2. add records in new lines in the following format: `<domain>:<ip>:<port>` (ex. `domain.xyz:127.0.0.1:3000`)

### Testing
How to test:
curl -H "Host: domain.xyz" -X GET http://localhost
