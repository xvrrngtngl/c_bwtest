
Measures download/upload bandwidth against a speedtest server and reports the
country of your public IP

## Build

    make

## Installation (Linux)

Install dependencies: `sudo apt install libcurl4-openssl-dev`

Build (from root directory): `make`

## Usage

    ./c_bwtest                # full test: location, best server, download, upload
    ./c_bwtest -l             # your location
    ./c_bwtest -s LOCATION    # best server in LOCATION   (e.g. -s Lithuania)
    ./c_bwtest -d HOST        # download test against HOST (e.g. -d speedtest.litnet.lt:8080)
    ./c_bwtest -u HOST        # upload test against HOST

The server list lives in `data/pretty_host_list.json` keyed by country
