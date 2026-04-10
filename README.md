# Booster Control Package

Este repositório contém o pacote `booster_control`, projetado para controlar robôs da Booster Robotics via ROS 2. Ele fornece uma interface entre comandos de alto nível (strings ou estados de controle remoto) e as APIs de baixo nível do robô.

## Componentes Principais

### 1. `booster_controller` (C++)
O nó principal que traduz comandos de ação para requisições de API (`RpcReqMsg`) que o robô entende.

- **Tópico de Entrada:** `/booster/action` (`std_msgs/msg/String`)
- **Tópico de Saída:** `LocoApiTopicReq` (`booster_msgs/msg/RpcReqMsg`)
- **Comandos Suportados:**
    - `kPrepare`, `kWalking`, `kSoccer`, `kLieDown`, `kGetUp`, `kShoot`, `kEnableRobocupWalkMode`, entre outros.

### 2. `actions.py` (Python)
Um script interativo CLI para enviar comandos de ação para booster_controller.

- **Uso:** Permite mapear teclas do teclado para comandos específicos (ex: 'E' para ativar o modo Robocup Walk).

### 3. `control.py` (Python)
Um simulador de controle Logitech F710.

- **Tópico de Saída:** `/remote_controller_state` (`booster_interface/msg/RemoteControllerState`)
- **Funcionalidade Adicional:** Quando a combinação **LT + A** (tecla '2') é pressionada, ele também publica o comando `kEnableRobocupWalkMode` no tópico `/booster/action`.

---

## Como Rodar

### Build
No diretório raiz do seu workspace colcon:
```bash
colcon build --packages-select booster_control
source install/setup.bash
```

### Execução

#### 1. Rodar o Controlador Principal
```bash
ros2 run booster_control booster_controller
```

#### 2. Rodar o Simulador de Controle (Joystick)
```bash
ros2 run booster_control control.py
```

#### 3. Rodar o Enviador de Ações Manual
```bash
ros2 run booster_control actions.py
```

---

## Detalhes das Teclas no Simulador (`control.py`)

- **1**: Simula LT + RT + A
- **2**: Simula LT + A (Ativa `kEnableRobocupWalkMode`)
- **3**: Simula LT + B
- **4**: Simula RT + A
- **q**: Sair
