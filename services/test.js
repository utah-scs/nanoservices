function async_call(req_id, service, func, args) {
    return new Promise(function(resolve, reject) {
        Call(req_id, service, func, 
	    function(error, result) {
                if (error) {
                    return reject(error);
                }
                return resolve(result);
            },
	    args);
    });
}

function test(req_id, args) {
    let post = "W4Trb9x7cr7IOSc03KqSBCZnBsYRdtMSLh6d1fcOd5KSjria6Y3L4Idjb5WGGYvqTYjqbgsdOfxdV68qcIr8Cld7f4ALx8fxpANvHMMkKINLum8nVVOzCgvPC01F088oDwxXmYcN1ZgDVh5Yq7cdHEJ8e05Vkoevy3fFja9q9YU9BNSoiURasi6rgh2qN3CcKrYVlQ12zF4OLLeH1kd7Ca027oTQQFdsejAiBSW0HTSe16OEd1mDU4RFqW0gkKNq @username_588 @username_784 @username_761 @username_50 @username_58 @username_17 http://gCZyQhpIIHUthlDvZwPBfFSn2WC7hnljloCBCzoqeJ0Po8WZrP6ushWyqd7bUAny http://WJ8vg0BavUPis6DBanTJ28N0a35b8dEnV9IpIecEzqzp3B4MqH99U3SyEvKp3BLW http://Twjjvu5Bt4MGijEmVetOwDggTAbmeXPsFvt8BieauN2LB6xMYd1o2XWzBBnj7cs4 http://64kTWmGuJ8GwT84trn4q9OVqVVlRzfUNYwURqjUT2xozPLRIy0jCYvMytWdh2GE6";
//    return async_call(req_id, "text_service.js", "upload_text", post)
    return async_call(req_id, "user.js", "login", args)
    .then(
       result => {
            let rep = new Object();
            rep._status = 200;
            rep._message = result;
            let ret = JSON.stringify(rep);
	    Reply(req_id, ServiceName, ret);}
    );
}
