var
    fs = require('fs'),
    punycode = require('punycode'),
    restler = require('restler'),
    punycode = require('punycode'),
    tld_module = require('tld.node'),
    _ = require("underscore");

var zones = [];

// Load top level domains from iana.org
function ensureTldReady(callback) {
    if (_.isEmpty(zones)) {
        restler.get("http://data.iana.org/TLD/tlds-alpha-by-domain.txt").on('complete', function (result) {
            var zl = result.split('\n');

            _.each(zl, function (z) {
                var domain = z.toLowerCase();

                if (domain.indexOf('#') != -1) return;

                if (domain.indexOf('xn--') != -1) {
                    try {
                        domain = punycode.toUnicode(domain);
                    } catch (err) {
                        console.log('ERR %s - %s', domain, err);
                    }
                }

                if (domain)
                    zones.push(domain);
            });

            // initialize module
            tld_module.load(zones, []);

            callback();
        });
    } else callback();
}

ensureTldReady(function () {
    // check domain
    var result = tld_module.tld("maps.google.com");

    console.log(result);
});


