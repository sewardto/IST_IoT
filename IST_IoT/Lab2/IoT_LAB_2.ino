#define TEMPERATURE A0
#define LIGHT A1
#define POTENTIOMETER A3

constexpr uint8_t yellow_led = 4;
constexpr uint8_t green_led = 3;
constexpr uint8_t red_led = 2;

bool p_led_state, t_sensor_state, t_led_state;
long previous_millis = 0;
int temperature_sample = 50, temperature_on_time, temperature_off_time;

void setup()
{
	Serial.begin(9600);
	pinMode(yellow_led, OUTPUT);
	pinMode(red_led, OUTPUT);
	pinMode(green_led, OUTPUT);
}

void loop()
{
	/*
	 *Task Temperature
	 */
	const int t_physic = (analogRead(TEMPERATURE) / 1024.0 * 5.0 - 0.5) * 100;
	Serial.println(t_physic);
	if (t_physic >= 26 && temperature_on_time < temperature_sample) {
		temperature_on_time++;
		if(temperature_off_time > 0)
			temperature_off_time--;
	}
	else if (t_physic >= 26 && temperature_on_time == temperature_sample && t_sensor_state == false) {
		t_led_state = true;
		t_sensor_state = true;
	}
	else if (t_physic < 26 && temperature_off_time < temperature_sample) {
		temperature_off_time++;
		if(temperature_on_time > 0)
			temperature_on_time--;
	}
	else if (t_physic < 26 && temperature_off_time == temperature_sample && t_sensor_state == true) {
		t_led_state = false;
		t_sensor_state = false;
	}
	digitalWrite(yellow_led, t_led_state);

	/*
	 * Task Light
	 */
	const int l = map(analogRead(LIGHT), 0, 1023, 255, 0);
	analogWrite(green_led, l);

	/*
	 * Task Potentiometer
	 */
	const int p = map(analogRead(POTENTIOMETER), 0, 1023, 100, 1000);
	const auto current_millis = millis();
	if (current_millis - previous_millis >= p) {
		previous_millis = current_millis;
		p_led_state = !p_led_state;
		digitalWrite(red_led, p_led_state);
	}
}