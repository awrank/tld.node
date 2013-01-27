var console = require('console'),
    vows = require('vows'),
    assert = require('assert'),
    fs = require('fs');


var suite = vows.describe('Testing dynamic TLD database updates');


// Test suite for main functionality of TLD module.
//
// There is list of test scenarios:
//      - when passing invalid domain
//		- when passing reserved (deprecated or blacklisted) domain (example.com)
//		- when passing unknown top level domain (unknown.domain)
//		- when passing active top level domain (www.google.com)
//		- when passing active top level domain with two words zone (www.pravda.com.ua)
//		- when passing active top level domain in utf-8 (магазин.рф)
//		- when passing active defined with wildcard for top level domain (*.ar)
//		- when passing exception for top level domain (uba.ar)

suite.addBatch({
    'tld-module': {
        topic: function() { 
        	var tld_module = require("../build/Release/module_tld.node"); 

			//var active_tld = fs.readFileSync("base.dat");
			//var reserved_tld  = fs.readFileSync("guide.dat");

        	var active_tld = ['com','net','ua','com.ua','net.ua','ru','*.ar','!uba.ar','рф'];
        	var reserved_tld = ['example.com'];
        	
        	tld_module.load(active_tld, reserved_tld);
        	
        	return tld_module;
        },
		
		'when passing unknown top level domain (scarlet.nl)': {
		    topic: function(tld_module) {
				return tld_module.tld('scarlet.nl');
		    },
		    'domain should be rejected with status "NOTFOUND"': function (result) {
		    	assert.equal (result.status, 4);
		    }
		},
		
		'when dynamicaly adding top level domain (nl)': {
		    topic: function(tld_module) {
				return tld_module.update('nl', 1 /* for tld-base */, 1 /* to add domain */);
		    },
		    'empty object has to be returned': function (result) {
		    	assert.ok(result);
		    }
		},
		
		'when passing active dynamicaly added top level domain (www.scarlet.nl)': {
		    topic: function(tld_module) {
				return tld_module.tld('www.scarlet.nl');
		    },
		    'domain should be validated with status "SUCCESS"': function (result) {
		    	assert.equal (result.status, 0);
		    },
		    'top level domain should be "nl"': function (result) {
		    	assert.equal (result.domain, 'nl');
		    },		    
		    'has to have two sub domains': function (result) {
		    	assert.equal (result.sub_domains.length, 2);
		    },
			'second sub domain should be "scarlet"': function (result) {
				assert.equal (result.sub_domains[1], 'scarlet');
			},
		    'first sub domain should be "www"': function (result) {
		    	assert.equal (result.sub_domains[0], 'www');
		    }
		}, 
						
		'when dynamicaly removing top level domain (com.ua)': {
		    topic: function(tld_module) {
				return tld_module.update('com.ua', 1 /* for tld-base */, 2 /* to remove domain */);
		    },
			'empty object has to be returned': function (result) {
				assert.ok(result);
			}
		},
		
		'when passing removed top level domain with two words zona (www.pravda.com.ua)': {
		    topic: function(tld_module) {
				return tld_module.tld('www.pravda.com.ua');
		    },
			'domain should be rejected with status "RESERVED"': function (result) {
				assert.equal (result.status, 1);
			}
		},

        'when dynamicaly removing top level domain (net.ua)': {
            topic: function(tld_module) {
                return tld_module.update('net.ua', 1 /* for tld-base */, 2 /* to remove domain */);
            },
            'empty object has to be returned': function (result) {
                assert.ok(result);
            }
        },

        'when passing removed top level domain with two words zona (www.deshevle.net.ua)': {
            topic: function(tld_module) {
                return tld_module.tld('www.deshevle.net.ua');
            },
            'domain should be rejected with status "RESERVED"': function (result) {
                assert.equal (result.status, 1);
            }
        },

		'when dynamicaly removing top level domain (ua)': {
		    topic: function(tld_module) {
				return tld_module.update('ua', 1 /* for tld-base */, 2 /* to remove domain */);
		    },
			'empty object has to be returned': function (result) {
				assert.ok(result);
			}
		},
		
		'when passing removed top level domain (rozetka.ua)': {
		    topic: function(tld_module) {
				return tld_module.tld('rozetka.ua');
		    },
			'domain should be rejected with status "RESERVED"': function (result) {
				assert.equal (result.status, 1);
			}
		},
		
		'when dynamicaly adding domain to list of reserved domains (example.net)': {
		    topic: function(tld_module) {
				return tld_module.update('example.net', 2 /* for reserved-base */, 1 /* to add domain */);
		    },
			'empty object has to be returned': function (result) {
				assert.ok(result);
			}
		},
		
		'when passing dynamicaly reserved domain (example.net)': {
		    topic: function(tld_module) {
				return tld_module.tld('example.net');
		    },
			'domain should be rejected with status "RESERVED"': function (result) {
				assert.equal (result.status, 1);
			}
		}
    }
});


suite.export(module);
