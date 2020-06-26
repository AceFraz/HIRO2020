import 'dart:convert';
import 'dart:io';

import 'package:flutter/material.dart';
import 'package:flutter_blue/flutter_blue.dart';
import 'package:path_provider/path_provider.dart';

void main() => runApp(MyApp());

class ProgressHUD extends StatelessWidget {
  final bool inAsyncCall;
  final double opacity;
  final Color color;
  final Animation<Color> valueColor;

  ProgressHUD({
    Key key,
    @required this.inAsyncCall,
    this.opacity = 0.3,
    this.color = Colors.grey,
    this.valueColor,
  }) : super(key: key);

  @override
  Widget build(BuildContext context) {
    List<Widget> widgetList = new List<Widget>();
    if (inAsyncCall) {
      final modal = new Stack(
        children: [
          new Opacity(
            opacity: opacity,
            child: ModalBarrier(dismissible: false, color: color),
          ),
          new Center(
            child: new CircularProgressIndicator(
              valueColor: valueColor,
            ),
          ),
        ],
      );
      widgetList.add(modal);
    }
    return Container();
  }
}

class MyApp extends StatelessWidget {
  @override
  Widget build(BuildContext context) => MaterialApp(
        title: 'BLE Demo',
        theme: ThemeData(
          primarySwatch: Colors.blue,
        ),
        home: StreamBuilder(
          stream: FlutterBlue.instance.state,
          initialData: BluetoothDeviceType.unknown,
          builder: (c, snapshot) {
            final state = snapshot.data;
            if (state == BluetoothState.on) {
              return MyHomePage(title: "microbit-hiro");
            }
            return BluetoothOffScreen();
          },
        ),
      );
}

class BluetoothOffScreen extends StatelessWidget {
  const BluetoothOffScreen({Key key, this.state}) : super(key: key);

  final BluetoothState state;

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Color(0xff89A1EF),
      body: Center(
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: <Widget>[
            Icon(
              Icons.bluetooth_disabled,
              size: 200.0,
              color: Colors.black,
            ),
            Text(
              'Stato Bluetooth : ${state != null ? state.toString().substring(15) : 'non disponibile'}.',
              style: TextStyle(color: Colors.black, fontSize: 15),
            ),
          ],
        ),
      ),
    );
  }
}

class MyHomePage extends StatefulWidget {
  MyHomePage({Key key, this.title}) : super(key: key);

  final String title;
  final FlutterBlue flutterBlue = FlutterBlue.instance;
  final List<BluetoothDevice> devicesList = new List<BluetoothDevice>();
  final Map<Guid, List<int>> readValues = new Map<Guid, List<int>>();

  @override
  _MyHomePageState createState() => _MyHomePageState();
}

class _MyHomePageState extends State<MyHomePage> {
  final _writeController = TextEditingController();
  BluetoothDevice _connectedDevice;
  List<BluetoothService> _services;

  _addDeviceTolist(final BluetoothDevice device) {
    if (!widget.devicesList.contains(device)) {
      setState(() {
        widget.devicesList.add(device);
      });
    }
  }

  @override
  void initState() {
    super.initState();
    widget.flutterBlue.connectedDevices
        .asStream()
        .listen((List<BluetoothDevice> devices) {
      for (BluetoothDevice device in devices) {
        _addDeviceTolist(device);
      }
    });
    widget.flutterBlue.scanResults.listen((List<ScanResult> results) {
      for (ScanResult result in results) {
        _addDeviceTolist(result.device);
      }
    });
    widget.flutterBlue.startScan();
  }

  String getDeviceState(BluetoothDeviceState state) {
    switch (state) {
      case BluetoothDeviceState.connecting:
        return "Connecting";
        break;
      case BluetoothDeviceState.disconnected:
        return "Disconnected";
        break;
      case BluetoothDeviceState.connected:
        return "Connected";
        break;
      case BluetoothDeviceState.disconnecting:
        return "Disconnecting";
        break;
      default:
        return "-1";
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text(widget.title),
      ),
      body: RefreshIndicator(
        child: SingleChildScrollView(
          child: Column(
            children: <Widget>[
              StreamBuilder<List<ScanResult>>(
                stream: FlutterBlue.instance.scanResults,
                initialData: [],
                builder: (c, snapshot) => Column(
                  children: snapshot.data.map((d) {
                    return Container(
                      padding: EdgeInsets.all(16),
                      height: 80,
                      child: Row(
                        children: <Widget>[
                          Expanded(
                            child: Text(d.device.name == ''
                                ? '(unknown device)'
                                : d.device.name),
                          ),
                          FlatButton(
                            color: Colors.blue,
                            child: StreamBuilder<BluetoothDeviceState>(
                                stream: d.device.state,
                                initialData: BluetoothDeviceState.disconnected,
                                builder: (c, snapshot) {
                                  return Text(
                                    getDeviceState(snapshot.data),
                                    style: TextStyle(color: Colors.white),
                                  );
                                }),
                            onPressed: () async {
                              widget.flutterBlue.stopScan();
                              await d.device.connect();
                              Navigator.of(context).pushAndRemoveUntil(
                                  MaterialPageRoute(
                                    builder: (context) =>
                                        MicroBitDeviceScreen(device: d.device),
                                  ),
                                  (route) => false);
                            },
                          ),
                        ],
                      ),
                    );
                  }).toList(),
                ),
              ),
            ],
          ),
        ),
        onRefresh: () =>
            FlutterBlue.instance.startScan(timeout: Duration(seconds: 4)),
      ),
    );
  }
}

class MicroBitDeviceScreen extends StatefulWidget {
  MicroBitDeviceScreen({Key key, this.device}) : super(key: key);

  BluetoothDevice device;

  @override
  _MicroBitDeviceScreenState createState() => _MicroBitDeviceScreenState();
}

class _MicroBitDeviceScreenState extends State<MicroBitDeviceScreen> {
  List<int> _read = List<int>();
  List<BluetoothCharacteristic> _characteristics =
      List<BluetoothCharacteristic>();
  bool _load = true;

  final writeController = TextEditingController();

  BluetoothCharacteristic characteristicWrite;
  BluetoothCharacteristic characteristicRead;

  String prova = "";

  File file;
  bool isFileTransfering = false;

  String line = "";

  @override
  void initState() {
    print('ciao');
    loadCharacteristic();
    super.initState();
  }

  Future<File> localFile() async {
    final directory = await getApplicationDocumentsDirectory();
    final path = directory.path;

    return File('$path/counter.txt');
  }

  Future<File> writeCounter(String counter) async {
    final file = await localFile();

    return file.writeAsString(counter);
  }

  Future<String> readCounter() async {
    try {
      String contents = await file.readAsString();
      final directory = await getExternalStorageDirectory();
      final path = directory.path;
      return contents + path;
    } catch (e) {
      return "-1";
    }
  }

  void loadCharacteristic() async {
    setState(() {
      _load = true;
    });

    List<BluetoothService> services = await widget.device.discoverServices();
    for (BluetoothService service in services) {
      for (BluetoothCharacteristic characteristic in service.characteristics) {
        _characteristics.add(characteristic);
        print(characteristic.uuid.toString());
        if (characteristic.uuid.toString() ==
            '6e400003-b5a3-f393-e0a9-e50e24dcca9e') {
          characteristicWrite = characteristic;
        }
        if (characteristic.uuid.toString() ==
            '6e400002-b5a3-f393-e0a9-e50e24dcca9e') {
          characteristicRead = characteristic;
        }
      }
    }
    setState(() {
      _load = false;
    });

    characteristicRead.value.listen(notifyHandler);
    await characteristicRead.setNotifyValue(true);
  }

  void notifyHandler(event) async {
    String msgEv = utf8.decode(event);
    if (msgEv == "fileTrStop") {
      isFileTransfering = false;
    }
    if (isFileTransfering) {
      file.writeAsString(msgEv, mode: FileMode.append);
      setState(() {
        prova = 'trasferimento file csv';
      });
    }

    if (msgEv == "tempo") {
      DateTime now = DateTime.now();
      String data = now.day.toString() +
          "," +
          now.month.toString() +
          "," +
          now.year.toString() +
          "," +
          now.hour.toString() +
          "," +
          now.minute.toString() +
          "," +
          now.second.toString();
      characteristicWrite.write(utf8.encode(data + ":"));
      setState(() {
        prova = 'configurazione orologio di sistema microbit';
      });
    }
    if (msgEv == "fileTr") {
      final directory = await getExternalStorageDirectory();
      final path = directory.path;
      print(path);
      file = File('$path/log_csv.csv');
      file.writeAsString("");
      isFileTransfering = true;
    }
    if (msgEv == 'occupato') {
      setState(() {
        prova =
            "il microbit sta raccogliendo i dati, per scambiare il file csv premi il tasto b del microbit e riavvia l'app";
      });
      await Future.delayed(Duration(seconds: 5));
      widget.device.disconnect();
      Navigator.of(context).pushAndRemoveUntil(
          MaterialPageRoute(
              builder: (context) => MyHomePage(
                    title: "microbit-hiro",
                  )),
          (route) => false);
    }
    if (msgEv == "stop") {
      setState(() {
        prova = "trasferimento terminato";
      });
      await Future.delayed(Duration(seconds: 2));
      final directory = await getExternalStorageDirectory();
      final path = directory.path;
      prova = '$path è il path dove è stato salvato il file csv';
      await Future.delayed(Duration(seconds: 4));
      widget.device.disconnect();
      Navigator.of(context).pushAndRemoveUntil(
          MaterialPageRoute(
              builder: (context) => MyHomePage(
                    title: "microbit-hiro",
                  )),
          (route) => false);
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text('Collegato a microbit'),
      ),
      body: Center(
        child: Text(prova),
      ),
    );
  }
}
