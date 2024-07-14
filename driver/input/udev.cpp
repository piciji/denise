
#include <unistd.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <libudev.h>

#include <linux/input.h>

#include <string>
#include <vector>
#include <cstring>

#include "../tools/hid.h"
#include "../tools/tools.h"
#include "../tools/crc32.h"

namespace DRIVER {
	
	struct Udev {
		udev* context = nullptr;
		udev_monitor* monitor = nullptr;
		udev_enumerate* enumerator = nullptr;
		udev_list_entry* devices = nullptr;

		struct JoypadInput {
			signed code;
			unsigned id;
			input_absinfo info;
		};

		struct Joypad {
			Hid::Joypad* hid = nullptr;
			
			int fd = -1;
			std::string deviceNode;

			std::vector<JoypadInput> axes;
			std::vector<JoypadInput> hats;
			std::vector<JoypadInput> buttons;			
		};
		std::vector<Joypad> joypads;

		auto init() -> bool {
			term();
			
			context = udev_new();
			if (!context) return false;

			monitor = udev_monitor_new_from_netlink(context, "udev");
			if (monitor) {
				udev_monitor_filter_add_match_subsystem_devtype(monitor, "input", nullptr);
				udev_monitor_enable_receiving(monitor);
			}

			enumerator = udev_enumerate_new(context);
			if (enumerator) {
				udev_enumerate_add_match_property(enumerator, "ID_INPUT_JOYSTICK", "1");
				udev_enumerate_scan_devices(enumerator);
				devices = udev_enumerate_get_list_entry(enumerator);
				for (udev_list_entry* item = devices; item != nullptr; item = udev_list_entry_get_next(item)) {
					auto name = udev_list_entry_get_name(item);
					udev_device* device = udev_device_new_from_syspath(context, name);
					auto deviceNode = udev_device_get_devnode(device);
					if (deviceNode) createJoypad(device, deviceNode);
					udev_device_unref(device);
				}
			}

			return true;
		}

		auto createJoypad(udev_device* device, const std::string& deviceNode) -> void {
			Joypad jp;
			jp.deviceNode = deviceNode;

			struct stat st;
			if (stat(deviceNode.c_str(), &st) < 0) return;
			
			jp.fd = open(deviceNode.c_str(), O_RDWR | O_NONBLOCK);
			if (jp.fd < 0) return;			

			uint8_t evbit[(EV_MAX + 7) / 8] = {0};
			uint8_t keybit[(KEY_MAX + 7) / 8] = {0};
			uint8_t absbit[(ABS_MAX + 7) / 8] = {0};

			ioctl(jp.fd, EVIOCGBIT(0, sizeof (evbit)), evbit); //all
			ioctl(jp.fd, EVIOCGBIT(EV_KEY, sizeof (keybit)), keybit); //buttons
			ioctl(jp.fd, EVIOCGBIT(EV_ABS, sizeof (absbit)), absbit); //axes

			#define testBit(buffer, bit) (buffer[bit >> 3] & 1 << (bit & 7))

			if (!testBit(evbit, EV_KEY))
				return (void)close(jp.fd);			
			
			udev_device* parent = udev_device_get_parent_with_subsystem_devtype(device, "input", nullptr);
			
			if (!parent)
				return (void)close(jp.fd);
			
			auto joyname = udev_device_get_sysattr_value(parent, "name");
			auto vendorId = udev_device_get_sysattr_value(parent, "id/vendor");
			auto productId = udev_device_get_sysattr_value(parent, "id/product");			
			
			udev_device* root = udev_device_get_parent_with_subsystem_devtype(parent, "usb", "usb_device");
						
			if (!root)
				return (void)close(jp.fd);
						
			auto devname = udev_device_get_devpath(root);													
			
			if(!devname)
				return (void)close(jp.fd);
			
			std::string buf(devname);
			buf.append(vendorId);
			buf.append(productId);
			
			jp.hid = new Hid::Joypad;			
			
			CRC32 crc32((uint8_t*)(buf.c_str()), buf.size());
			jp.hid->id = uniqueDeviceId( joypads, crc32.value() );
			std::string displayname = joyname ? (std::string)joyname : "Joypad";
			
			jp.hid->name = uniqueDeviceName(joypads, displayname);
			
			unsigned axes = 0;
			unsigned buttons = 0;
			unsigned hats = 0;

			for (signed i = 0; i < ABS_MISC; i++) {
				if (testBit(absbit, i)) {
					if (i >= ABS_HAT0X && i <= ABS_HAT3Y) {
						unsigned pos = i - ABS_HAT0X;
						unsigned hat = pos / 2;
						std::string dir = (hats % 2) == 0 ? ".X" : ".Y";
						jp.hats.push_back({i, hats});
						jp.hid->hats().append( std::to_string(hat) + dir );						
                        hats++;
						ioctl(jp.fd, EVIOCGABS(i), &jp.hats.back().info);
					} else {
						jp.axes.push_back({i, axes});
						ioctl(jp.fd, EVIOCGABS(i), &jp.axes.back().info);
						
						std::string axis = std::to_string(axes);

						switch (axes) {
							case 0: axis = "X"; break;
							case 1: axis = "Y"; break;
							case 2: axis = "Z"; break;
							case 3: axis = "Z|Rot"; break;
							case 4: axis = "X|Rot"; break;
							case 5: axis = "Y|Rot"; break;
						}
						jp.hid->axes().append(axis);												
                        axes++;
					}
				}
			}
			for (signed i = BTN_JOYSTICK; i < KEY_MAX; i++) {
				if (testBit(keybit, i)) {
					jp.buttons.push_back({i, buttons});
					jp.hid->buttons().append( std::to_string(buttons) );
                    buttons++;
				}
			}
			for (signed i = BTN_MISC; i < BTN_JOYSTICK; i++) {
				if (testBit(keybit, i)) {
					jp.buttons.push_back({i, buttons});
					jp.hid->buttons().append( std::to_string(buttons) );
                    buttons++;
				}
			}			
			
			joypads.push_back(jp);

			#undef testBit
		}

		auto term() -> void {
			if (enumerator) udev_enumerate_unref(enumerator);
			enumerator = nullptr;
			for (auto& jp : joypads) {
				if (jp.hid) delete jp.hid;
				close( jp.fd );
			}
				
			joypads.clear();
		}

		auto pollJoypad(std::vector<Hid::Device*>& devices) -> void {

			while (hotplugDevicesAvailable()) hotplugDevice();						

			for (auto& jp : joypads) {

				for(auto& input : jp.hid->buttons().inputs)
					input.oldValue = input.value;		
				
				for(auto& input : jp.hid->axes().inputs)
					input.oldValue = input.value;				
				
				for(auto& input : jp.hid->hats().inputs)
					input.oldValue = input.value;			
				
				input_event events[32];
				signed length = 0;
                
				while ((length = read(jp.fd, events, sizeof (events))) > 0) {
					length /= sizeof (input_event);
					for (unsigned i = 0; i < length; i++) {
						signed code = events[i].code;
						signed type = events[i].type;
						signed value = events[i].value;

						if (type == EV_ABS) {
							if (auto input = findCode(jp.axes, code)) {
								signed range = input->info.maximum - input->info.minimum;
								value = (value - input->info.minimum) * 65535ll / range - 32767;
								jp.hid->axes().inputs[input->id].value = sclamp<16>(value);
                                                                                                                                
							} else if (auto input = findCode(jp.hats, code)) {
								signed range = input->info.maximum - input->info.minimum;
								value = (value - input->info.minimum) * 65535ll / range - 32767;
								jp.hid->hats().inputs[input->id].value = sclamp<16>(value);
							}
						} else if (type == EV_KEY) {
							if (code >= BTN_MISC) {
								if (auto input = findCode(jp.buttons, code)) {									
									jp.hid->buttons().inputs[input->id].value = (bool)value;
								}
							}
						}
					}
				}

				devices.push_back(jp.hid);
			}
		}
		
		auto findCode(std::vector<JoypadInput>& inputs, signed code ) -> JoypadInput* {
			for(auto& input : inputs) {
				if(input.code == code) return &input;
			}
			return nullptr;
		}

		auto removeJoypad(udev_device* device, const std::string& deviceNode) -> void {
			for (unsigned n = 0; n < joypads.size(); n++) {
				if (joypads[n].deviceNode == deviceNode) {
					close(joypads[n].fd);
					if (joypads[n].hid) delete joypads[n].hid;
					joypads.erase(joypads.begin() + n);
					return;
				}
			}
		}

		auto hotplugDevice() -> void {
			udev_device* device = udev_monitor_receive_device(monitor);
			if (device == nullptr) return;

			auto value = udev_device_get_property_value(device, "ID_INPUT_JOYSTICK");
			auto action = udev_device_get_action(device);
			auto deviceNode = udev_device_get_devnode(device);
			
			if(!value || !action || !deviceNode) return;
			
			if ((std::string)value == "1") {
				if ((std::string)action == "add") {
					createJoypad(device, deviceNode);
				}
				else if ((std::string)action == "remove") {
					removeJoypad(device, deviceNode);
				}
			}
		}

		auto hotplugDevicesAvailable() -> bool {
			pollfd fd = {0};
			fd.fd = udev_monitor_get_fd(monitor);
			fd.events = POLLIN;
			return (::poll(&fd, 1, 0) == 1) && (fd.revents & POLLIN);
		}
		
		~Udev() {
			term();
		}
	};
}
