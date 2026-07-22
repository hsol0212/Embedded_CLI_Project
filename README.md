📟 STM32 FreeRTOS CLI & 실시간 모니터링 시스템

1. 프로젝트 개요 (Project Overview)

이 프로젝트는 STM32 MCU와 FreeRTOS 환경 기반에서 하드웨어 센서 제어, 실시간 데이터 모니터링, 그리고 사용자 명령어 처리(CLI)를 안정적으로 수행하기 위해 개발된 시스템입니다.

단순한 기능 구현을 넘어, RTOS 환경에서의 스택(Stack) 관리, 통신 병목 해결, 그리고 범용적인 프로토콜 설계에 집중하여 임베디드 시스템의 안정성과 확장성을 확보하는 데 주력했습니다.

개발 환경: STM32CubeIDE / VS Code (CMake, GCC)

하드웨어: STM32F411RE (Nucleo-64), DHT11, I2C LCD, 2-Axis Joystick, RTC

운영체제: FreeRTOS (CMSIS-RTOS V2)

핵심 기술: UART (IT/DMA), Timer (Microsecond delay), TLV Protocol, Stack Watermark Monitoring


2. 시스템 아키텍처 및 핵심 기능 (Key Features)

🚀 1) TLV 기반 실시간 모니터링 시스템 & 데이터 파싱

Qt 데스크톱 애플리케이션과의 연동을 위해 유연한 패킷 통신 시스템을 설계했습니다.

TLV (Tag-Length-Value) 프로토콜 적용: ID(Tag), Type(Length), Value 구조를 채택하여 추후 센서(조도, 자이로 등)가 추가되더라도 패킷 구조 변경 없이 하위 호환성을 유지하도록 설계했습니다.

유니버셜 메모리(Union) 활용: Float, Int, Boolean 등 다양한 타입의 센서 데이터를 4바이트 블럭 구조체(sensor_node_t)로 정형화하여 통신 효율을 극대화했습니다.

데이터 장전 & 발사 분리:

tempSystemTask(1초): 센서 데이터를 읽어 버퍼에 갱신(장전)

monitorSystemTask(20ms): 버퍼의 최신 스냅샷을 바이너리 형태로 터미널에 송신(발사)

모니터 뮤트(Mute) 로직: 모니터링 중에는 CLI 텍스트 로그(LOG_DBG)를 Bypass 시켜 데스크톱 파서의 에러를 원천 차단했습니다.

⚙️ 2) 체계적인 UART Log Level 시스템 구축

AUTOSAR 및 Linux 로그 표준을 벤치마킹하여 6단계(FATAL ~ VERBOSE) 로그 레벨을 도입했습니다.

컴파일 타임 & 런타임 필터링: 매크로를 이용해 배포 시 바이너리 크기를 최적화하고, CLI 명령어로 런타임 중 출력 레벨을 동적으로 변경할 수 있습니다.

ANSI Color Code: 터미널 출력 시 레벨별 직관적인 색상을 부여하여 디버깅 효율을 대폭 향상했습니다.

🛠️ 3) 하드웨어 정밀 제어 및 비동기 처리

1-Wire 프로토콜 제어 (DHT11): HAL_Delay(ms)의 한계를 극복하기 위해 APB1 버스의 TIM2를 활용, 1MHz(Microsecond) 단위의 정밀 타이머를 구성하여 40bit 센서 데이터를 성공적으로 수신했습니다.

ADC & DMA (Joystick): 2축 조이스틱의 아날로그 데이터를 CPU 개입 없이 DMA Circular 모드로 연속 수집하여 시스템 부하를 최소화했습니다.


3. Deep Dive & 트러블슈팅 (Troubleshooting)

프로젝트를 진행하며 발생한 RTOS 특유의 메모리 및 타이밍 이슈를 원인부터 분석하고 해결했습니다.

🚨 트러블슈팅 1: 부동소수점 포맷팅으로 인한 BusFault(PRECISERR) 해결

문제 상황: tempSystemTask 동작 중 실수형 데이터 포맷팅(%.2f) 및 LOG_DBG 호출 시 시스템이 다운되는 현상 발생.

원인 분석: Newlib/Newlib-nano 환경에서 부동소수점 변환 내부 함수(_dtoa_r) 및 로그 버퍼(char buf[256])가 할당된 스택(128 Words)을 초과하여 Stack Overflow가 발생했음을 확인했습니다.

해결 방안: 해당 Task의 스택 사이즈를 256 Words(1024 Bytes)로 확장하고, 다중 Task에서의 안전한 C 라이브러리 사용을 위해 FreeRTOS Newlib Reentrant 기능을 활성화하여 시스템 안정성을 확보했습니다.

🚨 트러블슈팅 2: UART Polling으로 인한 Task 선점과 스택 감소 현상 분석

문제 상황: tempStartTask 실행 시, 관계없는 타 Task들의 여유 스택(High Water Mark)이 일제히 감소(~200 Bytes)하는 현상 발견.

원인 분석: HAL_UART_Transmit (Polling) 함수가 2~3ms 동안 CPU를 블로킹함에 따라, 대기 중이던 타 Task들이 SysTick 인터럽트에 의해 강제 선점(Preemption) 당했습니다. 이 과정에서 ARM Cortex-M4F의 FPU 레지스터 및 일반 레지스터가 스택에 강제 백업(Push)되면서 여유 스택이 일시적으로 감소함을 파악했습니다.

해결 방안: 이는 메모리 릭(Leak)이 아닌 정상적인 RTOS 컨텍스트 스위칭의 흔적임을 규명하였으며, 근본적인 병목 해결을 위해 향후 UART 통신을 IT(Interrupt) 및 DMA 비동기 방식으로 리팩토링하는 계획을 수립했습니다.

🛡️ 스택 오버플로우 사전 감지 시스템 (Stack Overflow Hook) 구축

메모리 손상에 의한 조용한 에러(Silent Error)를 막기 위해 configCHECK_FOR_STACK_OVERFLOW를 활성화(Option 2: 패턴 마커 검사)했습니다.

vApplicationStackOverflowHook 콜백 함수를 구현하여, 오버플로우 발생 시 즉각적으로 해당 태스크의 이름을 UART로 출력하고 시스템을 무한 루프(안전 상태)에 빠뜨리는 안전망을 구축했습니다.


4. 개발 및 디버깅 환경 최적화

CMake 최적화: 컴파일 최적화 옵션 -Og -g3를 적용하여 매크로(Macro) 추적 등 디버깅 편의성과 실행 속도의 밸런스를 맞췄습니다.

Live Watch 연동: VS Code 환경에 liveWatch 속성과 svdPath를 연동하여 펌웨어 중단 없이 변수와 레지스터의 실시간 변동을 모니터링했습니다.

RTOS 인식 디버깅: serverRtos 설정을 통해 디버그 패널에서 FreeRTOS의 각 스레드(Task) 상태와 스택 사용량을 실시간으로 추적했습니다.
