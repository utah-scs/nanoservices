let counter = 0;
let current_timestamp = 0;

function get_counter(t) {
    if (current_timestamp > t) {
        print("Timestamps are not incremental.");
    }
    if (current_timestamp == t) {
        return counter++;
    } else {
        current_timestamp = t;
	counter = 0;
	return counter++;
    }
}

function upload_unique_id(req_id, post_type) {
    let t = Date.now();
    let idx = get_counter(t);
    let post_id = CoreID.toString() + t.toString() + idx.toString(); 
    print(post_id);
}
