# VPN over Steam

VPN over Steam is a solution for creating a secure network tunnel between clients and a server using Steam networking infrastructure.

It allows devices to be connected into a single virtual network, providing stable connectivity even behind NAT and firewalls — **without requiring the Steam client or Steam authentication**.

---

## 🚀 Features

- 🔒 Secure tunnel between clients and server  
- 🌐 NAT and firewall traversal out of the box  
- ⚡ Uses Steam networking infrastructure for reliable connectivity  
- 🧩 Easy integration into applications  
- 🚫 No need for:
  - Steam client  
  - Steam account  
  - Steam authentication  

---

## 🏗 How It Works

VPN over Steam uses the Steam Networking transport layer to establish connections between clients and the server.

- Direct connections are used when possible  
- Relay servers are used as fallback  
- NAT traversal is handled automatically  
- Reliable packet delivery is ensured  

All of this works **independently of the official Steam client**.

---

## 📦 Dependencies

This project uses Steam networking libraries extracted from **steamcmd**.

No Steam client installation is required.

---

## 🛠 Build

The project is built using **CMake**.

```bash
git clone <your-repo>
cd <your-repo>
mkdir build && cd build
cmake ..
cmake --build .
```

---

## ▶️ Usage

1. Start the server with arguments:
   - "-s",
   - "-li",
   - "0",
   - "-lp",
   - "MyPassword"

2. Connect clients:
   - "-c",
   - "-li",
   - "90284098090738699", // id from LogOnAnonymous: 90284098090738699
   - "-lp",
   - "MyPassword"

3. Traffic is automatically routed through the tunnel  

No additional configuration is required.

---

## 💡 Use Cases

- Multiplayer games  
- Peer-to-peer applications  
- Private networks  
- Bypassing network restrictions  
- Remote access  

---

## ⚙️ Advantages

- Simple setup  
- No dependency on Steam client  
- Stable connections  
- Minimal infrastructure requirements  

---

## ⚠️ Limitations

- Depends on Steam network availability  
- Possible latency when using relay servers  
