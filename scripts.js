//---------------------------------------------------------------------------

// These parameters describe the size of the dataset and the layout of a record.
var fieldSize = 4; // sizeof(Float64)
var fieldsPerRow = 32;
var recordSize = fieldsPerRow * fieldSize;
var bytes = 1024 * 1024 * 1024 * 32 / 1024 - 1;
var nRows = Math.floor(bytes / recordSize);
//var nRows = Math.floor(bytes/(32*4*2));
  
// This global variable stores the pointer to the hashtable.
var table;
var setup_done = 0;

// Setup hashtable and get the pointer to it.
function setup() {
  if (setup_done === 1)
    return;
  // Load Facebook social graph.
  LoadFBGraph();


  // Generating random data for evaluation
/*  for (var i = 0; i < nRows; ++i) {
    var buf = new ArrayBuffer(recordSize);
    // Uint32Array and Dataview have different endian.
    //var bufView = new DataView(buf);
    var bufView = new Uint32Array(buf);
    for (var j = 0; j < fieldsPerRow; ++j) {
      bufView[j] = Math.floor(Math.random() * 262142);
    }

    DBSet(i, buf);
  }
*/

  // The neural model trained from the UCI Wine dataset : https://archive.ics.uci.edu/ml/datasets/Wine
  var nn = new Float32Array([
-0.3956310424432262, -0.728396367329501, 0.6458884883211747, -0.053890504629200905, 0.5609231031697921, -0.014956443361235945, 0.6806618898367466, -1.126140378127827, -0.6050721716465759, 0.2496407107527968, 1.751573692921398, -0.45250985941629807, -0.34452067458452135, 0.3533251533547298, 6.847946214285789, 4.3501114797904235, 1.7176190825915933, 5.982450190846851, -3.7016620617081557, 1.1268756217922518, 0.35396508147463956, -2.70808197666614, -1.3463533035223412, 1.0192105136343137, 7.309758988421556, -4.524788867178, 1.0396818610531586, -6.104812691497341, -0.5537711160957713, -0.125479501453059, 0.8465127657613326, 0.5279176531833683, 1.584551609814106, 0.5227024034071397, 0.37873393510389913, 0.6570167074801447, 1.2048313561060764, 0.20523197352719919, 0.5477189359282952, 0.5097788105083177, 0.13804942709877555, 1.8827533678807447, 1.351547838332052, 0.561950915663477, -0.0881703822229495, -0.5237856027892672, -1.7996142826462704, -0.03713048866165729, -0.4868635576315739, 4.009428618372302, 0.5603490191789772, 1.1190506333135117, -1.7640964715171359, 1.8079382426138624, 2.7568704375773008, -2.333698948757055, 0.4757094682312288, 0.0013936637887628932, -1.0879391288492364, -1.6306308943933094, -1.1429381075526672, -0.5176415418958816, 0.12777009586061444, 5.5607786717067595, 0.596175031771659, 1.2907872705205843, -3.527457373430356, 2.7841606285888436, 4.275687203652454, -0.4974075923367603, -3.0243616487322957, 9.111097096037945, -5.59054306291906, 4.9843510777945985, 3.8886550050428457, -5.844744904505689, 1.7010114674812975, 5.312662242897756, 0.9701950097203021, -6.055701409766454, -7.067082359633793, 0.7923834325384879, -1.028987583230588, -12.410237575392896, 1.7918830416983675, -0.1989312770511396, 3.8678207804398674, 2.0153171695111283
]);

/*
  // The neural model trained from the UCI Iris dataset : https://archive.ics.uci.edu/ml/datasets/Iris
  var nn = new Float32Array([
-2.944215765444462, -3.2112169234979393, 13.515167097957487, 9.567231293783022, -12.347720875159638, -2.0407837761024084, 4.414471154379188, -6.328835782209486, -6.586523816611092, 2.048639195975654, -10.189594058390842, -9.781563982426805, 5.084495659302524, 10.943023697767737, -4.466292652363755, -5.4641576242307215, -8.70718157712314, 8.118555848632559, -3.6158609254807015
]);
*/

  var buf = nn.buffer;
  DBSet(10000, buf);

  // One normalized input for neural inference (Wine dataset)
  var input = new Float32Array([0.4008559201141227, 0.5631578947368422, 0.36561264822134387, 0.5401069518716578, 0.48453608247422686, 0.5434782608695652, 0.23103448275862068, 0.07172995780590717, 0.7547169811320755, 0.3312302839116719, 0.6843003412969284, 0.0975609756097561, 0.12820512820512825]);

  // Input for neural inference (Iris dataset)
  // var input = new Float32Array([0.1666666666666668, 0.41666666666666663, 0.06779661016949151, 0.04166666666666667]);
  var input_buf = input.buffer;
  DBSet(10001, input_buf);
 
  table = GetHashTable();
  setup_done = 1;
  return "+OK\r\n";
}

// Parameters for inference (Wine dataset)
var num_inputs = 13;
var num_hidden_weights = num_inputs + 1;
var num_hiddens = 5;
var num_outputs = 3;
var num_output_weights = num_hiddens + 1;
var nnBuf = new ArrayBuffer((num_hidden_weights*num_hiddens + num_output_weights*num_outputs) * 4);
var nnInput = new ArrayBuffer(num_inputs*4);

// Parameters for inference (Iris dataset)
/*var num_inputs = 4;
var num_hidden_weights = num_inputs + 1;
var num_hiddens = 2;
var num_outputs = 3;
var num_output_weights = num_hiddens + 1;
var nnBuf = new ArrayBuffer((num_hidden_weights*num_hiddens + num_output_weights*num_outputs) * 4);
var nnInput = new ArrayBuffer(num_inputs*4);
*/

function predict() {
    var p = HTGet(table, 10000, nnBuf);
    var dv = new Float32Array(nnBuf);
    var x = HTGet(table, 10001, nnInput);
    var input = new Float32Array(nnInput);

    var old_input = input;
    var new_input = [];

    for (var i = 0; i < num_hiddens; i++) {
        var index = num_hidden_weights*i;
        var activation = dv[index + num_hidden_weights - 1];
        for (var j = 0; j < num_hidden_weights - 1; j++) {
            activation = activation + dv[index + j]*old_input[j];
        }
        new_input.push(1.0/(1.0 + Math.exp(-1*activation)));
    }
    old_input = new_input;

    new_input = [];
    for (var i = 0; i < num_outputs; i++) {
        var index = num_hidden_weights*num_hiddens + i * num_output_weights;
        var activation = dv[index + num_output_weights - 1];
        for (var j = 0; j < num_output_weights - 1; j++) {
            activation = activation + dv[index + j]*old_input[j];
        }
        new_input.push(1.0/(1.0 + Math.exp(-1*activation)));
    }
    old_input = new_input;
    var res = old_input.indexOf(Math.max(...old_input));
    return res;
}

// C++ binding version
/*function get(key) {
  var p = DBGet(key%(nRows - 1));
  var dv = new Uint32Array(p);

  return dv;
}
*/

// CSA version
var getBuf = new ArrayBuffer(recordSize);
function get(key) {
  var length = HTGet(table, key%(nRows - 1), getBuf);
  var dv = new Uint32Array(getBuf);
  return dv;
}

// C++ binding version of count_friend_list
/*function count_friend_list(key, depth) {
     var k = key;
     var p = DBGet(k);
     var l = p.byteLength/4;
     if (l == 0)
         return 0; 
     var sum = l;
     if (depth == 1)
        return sum;
         
     var dv = new Uint32Array(p);
     for (var i = 0; i < l; i++) {
         k = dv[i];
         sum = sum + count_friend_list(k, depth - 1);
     }
    return sum;
}*/

// CSA version of count_friend_list
var tmpBuf = new ArrayBuffer(8000);
function count_friend_list(key, depth) {
    var k = key;
    if (depth == 1) {
        var l = HTGet(table, k, tmpBuf);
    } else {
        var friendBuf = new ArrayBuffer(0);
        var l = HTGet(table, k, friendBuf);
    }
    if (l === undefined) {
        return 0;
    }
    var sum = l/4;
    if (depth == 1)
        return sum;

       
    for (var i = 0; i < l/4; i++) {
        k = HTGetField(friendBuf, i);
        sum = sum + count_friend_list(k, depth - 1);
    }
    return sum;
}

function hello() {
    print("hello\n");
    return;
}

function test() {
    print("hello\n");
    Call("hello");
    return "OK\n";
}
