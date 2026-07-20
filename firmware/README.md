## Firmware

## Current functionality

- PWM motor speed control (TIM2)
- Direction control using DRV8876 (PH/EN mode)
- UART command interface (USART2)
- Supported commands:
  - L20 - rotate left with 20% duty cycle
  - P50 - rotate right with 50% duty cycle
  - S - stop motor

## Current Measurement

Implemented current measurement using DRV8876 IPROPI output.

Configuration:
- ADC1 input: PC0
- IPROPI resistor: 2.47 kΩ (measured on PCB)
- AIPROPI: 1000 µA/A (datasheet)
- Filtering capacitor: 220 µF (testing)

Conversion:

I_motor = V_CS / 2.47

Tests:
- PWM 20% → 65 mA
- PWM 40% → 85 mA
- PWM 60% → 105 mA
- PWM 80% → 125 mA
- PWM 100% → 135 mA
