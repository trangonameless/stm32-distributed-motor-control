# Firmware

## Current functionality

* Closed-loop DC motor speed control (PI controller, no derivative term)
* Quadrature encoder speed feedback (TIM3, encoder mode)
* PWM motor drive (TIM2)
* Direction control using DRV8876 (PH/EN mode)
* Current measurement using DRV8876 IPROPI
* Hardware + software fault diagnostics, with automatic motor shutdown
* UART command interface (USART1)
* UART telemetry stream to Raspberry Pi (USART1), JSON-formatted

## UART command interface

Commands are terminated by `\r` or `\n`:

| Command   | Description                                              |
|-----------|-------------------------------------------------------------|
| `L<rpm>`  | Rotate left, target speed `<rpm>` (clamped 0–240 RPM)        |
| `P<rpm>`  | Rotate right, target speed `<rpm>` (clamped 0–240 RPM)       |
| `S`       | Stop motor, disable control, reset PI integrator             |
| `R`       | Clear latched diagnostic faults                              |

> Note: commands take a target **speed in RPM**, not a raw PWM duty
> cycle — the PI controller computes the duty cycle needed to reach it.
> (Earlier prototype versions accepted a duty-cycle percentage directly,
> e.g. `L20`/`P50`; this was replaced once closed-loop speed control was
> implemented.)

## Speed feedback (encoder)

* Encoder: quadrature, 924 counts/revolution (Waveshare 22346 datasheet)
* Sampled every 10 ms (TIM3 counter delta → RPM)
* 16-bit counter wrap-around handled explicitly (signed delta in native
  register width), so continuous rotation doesn't produce spurious RPM
  spikes every ~70 revolutions

## Speed control (PI)

* Closed-loop PI controller, update period 10 ms
* Output clamped to 0–100% PWM duty
* Anti-windup: integral term only accumulates while output is within
  the 0–100 range
* Tuned gains: Kp = 1.1, Ki = 0.4

## Current measurement

Implemented using DRV8876 IPROPI output.

Configuration:
* ADC1 input: PC0
* IPROPI resistor: 2.47 kΩ (measured on PCB)
* AIPROPI: 1000 µA/A (datasheet)
* Filtering capacitor: 220 µF (testing)

Conversion:

I_motor = V_CS / 2.47


Tests (open-loop PWM, pre-PI-controller):
* PWM 20% → 65 mA
* PWM 40% → 85 mA
* PWM 60% → 105 mA
* PWM 80% → 125 mA
* PWM 100% → 135 mA

## Diagnostics

Faults are reported as a bitmask (`uint8_t`), so multiple faults can be
active simultaneously:

| Value  | Fault                     | Trigger condition                                                         | Debounce |
|--------|----------------------------|------------------------------------------------------------------------------|----------|
| `0x00` | `MOTOR_FAULT_NONE`         | No active fault                                                               | —        |
| `0x01` | `MOTOR_FAULT_STALL`        | High PWM + high setpoint, but measured RPM stays near zero                   | 200 ms   |
| `0x02` | `MOTOR_FAULT_OVERCURRENT`  | DRV8876 nFAULT pin asserted (hardware overcurrent/overvoltage protection)     | 10 ms    |
| `0x04` | `MOTOR_FAULT_ENCODER`      | Measured RPM exceeds a physically implausible value (sensor fault)           | 100 ms   |
| `0x08` | `MOTOR_TRACKING_ERROR`     | Measured speed falls significantly short of setpoint                         | 500 ms   |

All faults use debounced detection (a condition must hold for a minimum
duration before latching) to avoid false positives from transient
noise. Any latched fault immediately stops the motor and disables
control; faults are cleared via the `R` UART command.

The driver overcurrent check is hardware-driven (DRV8876's internal
comparator) and is monitored unconditionally, independent of whether
the motor is currently enabled.

## Telemetry

Sent over UART every 250 ms as a JSON line:

```json
{"current":<mA>,"rpm":<RPM>,"pwm":<0-100>,"set_speed":<RPM>}
```

Together with the UART command interface above, this forms a
bidirectional link with the Raspberry Pi supervisor: the Pi sends
speed/direction commands, and the STM32L476 streams back live motor
state. Consumed by Raspberry Pi and visualized in Grafana (current,
measured speed, PWM duty, and commanded setpoint over time).
