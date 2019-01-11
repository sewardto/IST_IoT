#define LED_NU(x) 1 + (x) % 5

constexpr auto green_led = 5;
constexpr auto red_led = 4;
constexpr auto blue_led = 3;
constexpr auto yellow_led = 2;
constexpr auto all_off = 1;
constexpr auto button = 6;
constexpr long gap_time = 1000;
constexpr unsigned button_sample = 50;
uint8_t id;
uint8_t press_time;
bool button_state;
bool flag;

inline void turnledon(const uint8_t nu)
{
	if (nu == all_off)
		return;
	digitalWrite(nu, HIGH);
}

inline void turnoff(const uint8_t nu)
{
	digitalWrite(nu, LOW);
}

void setup()
{
	Serial.begin(9600);
	pinMode(green_led, OUTPUT);
	pinMode(red_led, OUTPUT);
	pinMode(blue_led, OUTPUT);
	pinMode(yellow_led, OUTPUT);
	pinMode(button, INPUT_PULLUP);
}

void loop()
{
	if (flag) {
		turnoff(LED_NU(id++));
		turnledon(LED_NU(id));
	}
	for (auto end_time = millis(), start_time = millis(); end_time - start_time < gap_time; end_time = millis()) {
		const auto button_input = digitalRead(button);
		if (button_input == LOW && press_time < button_sample)
			press_time++;
		else if (button_state == LOW && press_time == button_sample && button_state == false) {
			flag = !flag;
			button_state = true;
		}
		else if (button_state != LOW && press_time > 0)
			press_time--;
		else if (button_input != LOW && press_time == 0 && button_state == true)
			button_state = false;
	}
}
