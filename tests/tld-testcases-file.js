var console = require('console'),
    vows = require('vows'),
    assert = require('assert'),
    fs = require('fs');


var suite = vows.describe('Testing scenarious on TLD database loaded from file(s)');


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

		var active_tld = fs.readFileSync("base.dat");
		var reserved_tld  = fs.readFileSync("guide.dat");

        	tld_module.load(active_tld, reserved_tld);
        	
        	return tld_module;
        },
        
        'when passing invalid domain': {
            topic: function(tld_module) {
				return tld_module.tld('invalid domain');
            },
            'domain should be rejected with status "ILLEGAL"': function (result) {
            	assert.equal (result.status, 2);
            }
        },
        
		'when passing reserved (depricated or blacklisted) domain (example.com)': {
		    topic: function(tld_module) {
				return tld_module.tld('example.com');
		    },
		    'domain should be rejected with status "RESERVED"': function (result) {
		    	assert.equal (result.status, 1);
		    }
		},
		
		'when passing unknown top level domain (unknown.domain)': {
		    topic: function(tld_module) {
				return tld_module.tld('unknown.domain');
		    },
		    'domain should be rejected with status "NOTFOUND"': function (result) {
		    	assert.equal (result.status, 4);
		    }
		},
		
		'when passing active top level domain (www.google.com)': {
		    topic: function(tld_module) {
				return tld_module.tld('www.google.com');
		    },
		    'domain should be validated with status "SUCCESS"': function (result) {
		    	assert.equal (result.status, 0);
		    },
		    'top level domain should be "com"': function (result) {
		    	assert.equal (result.domain, 'com');
		    },		    
		    'has to have two sub domains': function (result) {
		    	assert.equal (result.sub_domains.length, 2);
		    },
			'second sub domain should be "google"': function (result) {
				assert.equal (result.sub_domains[1], 'google');
			},
		    'first sub domain should be "www"': function (result) {
		    	assert.equal (result.sub_domains[0], 'www');
		    }
		},
		
		'when passing active top level domain with two words zona (www.pravda.com.ua)': {
		    topic: function(tld_module) {
				return tld_module.tld('www.pravda.com.ua');
		    },
		    'domain should be validated with status "SUCCESS"': function (result) {
		    	assert.equal (result.status, 0);
		    },
		    'top level domain should be "com.ua"': function (result) {
		    	assert.equal (result.domain, 'com.ua');
		    },		    
		    'has to have two sub domains': function (result) {
		    	assert.equal (result.sub_domains.length, 2);
		    },
			'second sub domain should be "pravda"': function (result) {
				assert.equal (result.sub_domains[1], 'pravda');
			},
		    'first sub domain should be "www"': function (result) {
		    	assert.equal (result.sub_domains[0], 'www');
		    }
		},
		
		
		'when passing active top level domain in utf-8 (магазин.рф)': {
		    topic: function(tld_module) {
				return tld_module.tld('магазин.рф');
		    },
		    'domain should be validated with status "SUCCESS"': function (result) {
		    	assert.equal (result.status, 0);
		    },
			'top level domain should be "рф"': function (result) {
				assert.equal (result.domain, 'рф');
			},		    
			'has to have one sub domain': function (result) {
				assert.equal (result.sub_domains.length, 1);
			},
			'sub domain should be "магазин"': function (result) {
				assert.equal (result.sub_domains[0], 'магазин');
			}
		},
		
		'when passing active defined with wildcard for top level domain (*.ar)': {
		    topic: function(tld_module) {
				return tld_module.tld('northlands.org.ar');
		    },
		    'domain should be validated with status "SUCCESS"': function (result) {
		    	assert.equal (result.status, 0);
		    },
			'top level domain should be "org.ar"': function (result) {
				assert.equal (result.domain, 'org.ar');
			},		    
			'has to have one sub domain': function (result) {
				assert.equal (result.sub_domains.length, 1);
			},
		    'sub domain should be "northlands"': function (result) {
		    	assert.equal (result.sub_domains[0], 'northlands');
		    }
		},		
		
		'when passing exception for top level domain (uba.ar)': {
		    topic: function(tld_module) {
				return tld_module.tld('uba.ar');
		    },
		    'domain should be validated with status "SUCCESS"': function (result) {
		    	assert.equal (result.status, 0);
		    },
			'top level domain should be "ar"': function (result) {
				assert.equal (result.domain, 'ar');
			},		    
			'has to have one sub domain': function (result) {
				assert.equal (result.sub_domains.length, 1);
			},
			'sub domain should be "uba"': function (result) {
				assert.equal (result.sub_domains[0], 'uba');
			}
		}		
    }
});

suite.export(module);
