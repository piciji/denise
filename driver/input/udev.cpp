
#include <unistd.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <libudev.h>
#include <sys/inotify.h>
#include <dirent.h>

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
		int inotifyFd = -1;

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
			if (!context)
				return false;

			monitor = udev_monitor_new_from_netlink(context, "udev");
			if (!monitor)
				return false;

			if (!inSandbox()) {
				udev_monitor_filter_add_match_subsystem_devtype(monitor, "input", nullptr);
				udev_monitor_enable_receiving(monitor);

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
						//udev_device_unref(device);
					}
				}
			} else {
				genericJoystickDetection();

				inotifyFd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);

				if (inotifyFd >= 0) {
					if (inotify_add_watch(inotifyFd, "/dev/input", IN_CREATE | IN_DELETE | IN_MOVE | IN_ATTRIB) < 0) {
	  					close(inotifyFd);
	  					inotifyFd = -1;
					}
				}
			}			

			return true;
		}

		auto inSandbox() -> bool {
			if (access("/.flatpak-info", F_OK) == 0)
				return true;
		
			if (getenv("SNAP") != nullptr && getenv("SNAP_NAME") != nullptr && getenv("SNAP_REVISION") != nullptr)
				return true;
		
			if (access("/run/host/container-manager", F_OK) == 0)
				return true;
		
			return false;
		}

		auto createJoypad(udev_device* device, const std::string& deviceNode) -> void {
			Joypad jp;
			jp.deviceNode = deviceNode;
			struct stat st;

			for (unsigned n = 0; n < joypads.size(); n++) {
				if (joypads[n].deviceNode == deviceNode)
					return;
			}

			if (stat(deviceNode.c_str(), &st) < 0)
				return;
			
			jp.fd = open(deviceNode.c_str(), O_RDONLY | O_NONBLOCK);
			if (jp.fd < 0)
				return;

			if (!device) {
				device = getDevice(jp.fd);
				if (!device)
					return (void)close(jp.fd);
			}

			uint8_t evbit[(EV_MAX + 7) / 8] = {0};
			uint8_t keybit[(KEY_MAX + 7) / 8] = {0};
			uint8_t absbit[(ABS_MAX + 7) / 8] = {0};

			ioctl(jp.fd, EVIOCGBIT(0, sizeof (evbit)), evbit); //all
			ioctl(jp.fd, EVIOCGBIT(EV_KEY, sizeof (keybit)), keybit); //buttons
			ioctl(jp.fd, EVIOCGBIT(EV_ABS, sizeof (absbit)), absbit); //axes

			#define testBit(buffer, bit) (buffer[bit >> 3] & 1 << (bit & 7))

			if (!testBit(evbit, EV_KEY))
				return close(jp.fd), (void)udev_device_unref(device);
			
			udev_device* parent = udev_device_get_parent_with_subsystem_devtype(device, "input", nullptr);
			
			if (!parent)
				return close(jp.fd), (void)udev_device_unref(device);
			
			auto joyname = udev_device_get_sysattr_value(parent, "name");
			auto vendorId = udev_device_get_sysattr_value(parent, "id/vendor");
			auto productId = udev_device_get_sysattr_value(parent, "id/product");			
			
			const char* devname = nullptr;
			udev_device* root = udev_device_get_parent_with_subsystem_devtype(parent, "usb", "usb_device");
						
			if (!root) { // maybe Bluetooth
				root = udev_device_get_parent_with_subsystem_devtype(parent, "hid", nullptr);
				if (!root)
					root = parent;

				devname = udev_device_get_sysattr_value(root, "uevent");
			} else
				devname = udev_device_get_devpath(root);
			
			if(!devname)
				return close(jp.fd), (void)udev_device_unref(device);
			
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

			udev_device_unref(device);
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

			if (monitor) {
				udev_monitor_unref(monitor);
				monitor = nullptr;
			}

			if (context) {
				udev_unref(context);
				context = nullptr;
			}

			if (inotifyFd >= 0) {
				close(inotifyFd);
				inotifyFd = -1;
			}							
		}

		auto pollJoypad(std::vector<Hid::Device*>& devices) -> void {

			if (inotifyFd >= 0)
				notifyJoystickDetection();
			else {
				while (hotplugDevicesAvailable())
					hotplugDevice();
			}

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

		auto removeJoypad(const std::string& deviceNode) -> void {
			for (unsigned n = 0; n < joypads.size(); n++) {
				if (joypads[n].deviceNode == deviceNode) {
					close(joypads[n].fd);
					if (joypads[n].hid)
						delete joypads[n].hid;
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
					removeJoypad(deviceNode);
				}
			}
		}

		auto hotplugDevicesAvailable() -> bool {
			pollfd fd = {0};
			fd.fd = udev_monitor_get_fd(monitor);
			fd.events = POLLIN;
			return (::poll(&fd, 1, 0) == 1) && (fd.revents & POLLIN);
		}

		static auto isJoyEvent(const std::string& path) -> bool {
			if (!startsWith(path, "event"))
				return false;

			std::string suf = path.substr(5);  
			return isNumber(suf);
		}

		auto notifyJoystickDetection() -> void {
			char buffer[sizeof(struct inotify_event) + FILENAME_MAX + 1];			
			ssize_t bytes = read(inotifyFd, &buffer, sizeof(buffer));
			char* ptr = &buffer[0];

			while(bytes > 0) {
				struct inotify_event* event = (inotify_event*)ptr;
				if (event->len && isJoyEvent((std::string)event->name)) {
					std::string path = "/dev/input/" + (std::string)event->name;

					if (event->mask & (IN_CREATE | IN_MOVED_TO | IN_ATTRIB)) {
						createJoypad(nullptr, path);
					} else if (event->mask & (IN_DELETE | IN_MOVED_FROM)) {
						removeJoypad(path);
					}
				}

				ssize_t len = sizeof(struct inotify_event) + event->len;
				bytes -= len;
				ptr += len;
			}
		}

		static auto scanFilter(const struct dirent* entry) -> int {
			return isJoyEvent(entry->d_name);
		}

		auto genericJoystickDetection() -> void {
			struct dirent** entries = nullptr;

			int count = scandir("/dev/input", &entries, scanFilter, nullptr);

			for (int i = 0; i < count; i++) {
				std::string path = "/dev/input/" + (std::string)entries[i]->d_name;
				createJoypad(nullptr, path);
				free(entries[i]);
			}

			free(entries);
		}

		auto getDevice(int fd) -> udev_device* {
			struct stat sb;
			char type;

			if (fstat(fd, &sb) == -1)
				return nullptr;

			if (S_ISBLK(sb.st_mode))
				type = 'b';
			else if (S_ISCHR(sb.st_mode))
				type = 'c';
			else
				return nullptr;

			return udev_device_new_from_devnum(context, type, sb.st_rdev);
		}
		
		~Udev() {
			term();
		}
	};
}
