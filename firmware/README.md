## Firmware

## Current functionality

- PWM motor speed control (TIM2)
- Direction control using DRV8876 (PH/EN mode)
- UART command interface (USART2)
- Supported commands:
  - L20 - rotate left with 20% duty cycle
  - P50 - rotate right with 50% duty cycle
  - S - stop motor
