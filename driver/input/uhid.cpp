
#include <sys/param.h>
#include <sys/stat.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

//#include <usb.h>
//#include <dev/usb/usb.h>
#include <dev/usb/usbhid.h>

#include <usbhid.h>

//#include <osreldate.h>

#include <dev/usb/usb_ioctl.h>

//#include <sys/joystick.h>

#include "../tools/hid.h"
#include "../tools/tools.h"
#include "../tools/crc32.h"

namespace DRIVER {
	
struct Uhid {

	struct JoypadInput {
		int code;
		unsigned id;
	};

	struct Report {
		int id = -1;
		size_t size;
		uint8_t* data = nullptr;
	};


	struct Joypad {
		Hid::Joypad* hid = nullptr;
		
		int fd = -1;
		std::string deviceNode;
		report_desc* desc = nullptr;
		Report report;

		std::vector<JoypadInput> axes;
		std::vector<JoypadInput> hats;
		std::vector<JoypadInput> buttons;			
	};
	std::vector<Joypad> joypads;

	auto init() -> bool {
		std::string node;
		struct stat st;

		for (int joy = 0; joy < 16; joy++) {
			Joypad jp;
			node = "/dev/uhid" + std::to_string(joy);

			if (stat(node.c_str(), &st) == -1)
    			continue;

			if (!createJoypad(node, jp)) {
				free(jp);
				continue;
			}
printf("joy ok: %s \n", node.c_str());
			usb_device_info di;
			std::string buf = "";
			std::string joyName = "";

#ifdef USB_GET_DEVICEINFO
        	if (ioctl(jp.fd, USB_GET_DEVICEINFO, &di) != -1) {
        		std::string releaseNo = std::to_string(di.udi_releaseNo);
        		std::string vendorNo = std::to_string(di.udi_vendorNo);
        		std::string productNo = std::to_string(di.udi_productNo);

        		std::string vendor = std::string(di.udi_vendor);
        		std::string product = std::string(di.udi_product);

        		// printf("joy name: %i \n", product); fflush(NULL);

        		if (vendor != "")
					buf.append(vendor);

				if (product != "") {
					buf.append(product);
					joyName = product;
				}
				
				buf.append(releaseNo);
				buf.append(vendorNo);
				buf.append(productNo);
	        }
#endif
	        if (buf == "")
	        	buf.append(node);

	        if (joyName == "")
	        	joyName = "Joypad";
		
			jp.hid->name = uniqueDeviceName(joypads, joyName);
			CRC32 crc32((uint8_t*)(buf.c_str()), buf.size());
			jp.hid->id = uniqueDeviceId( joypads, crc32.value() );

			joypads.push_back(jp);
		}

		return true;
	}


	auto createJoypad(std::string& node, Joypad& jp) -> bool {		
		jp.deviceNode = node;
	    hid_item item;
	    hid_data* hdata;
	    
	    unsigned buttons = 0;
	    unsigned hats = 0;
	    unsigned axes = 0;

	    jp.fd = open(node.c_str(), O_RDONLY | O_CLOEXEC);
	    if (jp.fd == -1)
	        return false;
	    
	    jp.desc = hid_get_report_desc(jp.fd);
	    if (!jp.desc)
	        return false;
	    
	#ifdef __FreeBSD__
	    jp.report.id = hid_get_report_id(jp.fd);
	    if (jp.report.id < 0)
	#else
	    if (ioctl(jp.fd, USB_GET_REPORT_ID, &jp.report.id) < 0)
	#endif
	        jp.report.id = -1;


    	jp.report.size = hid_report_size(jp.desc, hid_input, jp.report.id);
    	if (jp.report.size <= 0)
    		return false;

    	jp.report.data = new uint8_t[jp.report.size];
	    
//	#ifdef __FreeBSD__
	    hdata = hid_start_parse(jp.desc, 1 << hid_input, jp.report.id);
	//#else
	  //  hdata = hid_start_parse(jp.desc, 1 << hid_input);
//	#endif

	    if (!hdata)
	        return false;

		jp.hid = new Hid::Joypad;

	    while (hid_get_item(hdata, &item) > 0) {
			if (item.kind != hid_input)
				continue;


			if (HID_PAGE(item.usage) == HUP_GENERIC_DESKTOP) {
				int usage = HID_USAGE(item.usage);

				switch(usage) {
					case HUG_X:
						jp.axes.push_back({usage, axes++});
						jp.hid->axes().append("X");
						break;
					case HUG_Y:
						jp.axes.push_back({usage, axes++});
						jp.hid->axes().append("Y");
						break;
					case HUG_Z:
						jp.axes.push_back({usage, axes++});
						jp.hid->axes().append("Z");
						break;
					case HUG_RX:
						jp.axes.push_back({usage, axes++});
						jp.hid->axes().append("X|Rot");
						break;
					case HUG_RY:
						jp.axes.push_back({usage, axes++});
						jp.hid->axes().append("Y|Rot");
						break;
					case HUG_RZ:
						jp.axes.push_back({usage, axes++});
						jp.hid->axes().append("Z|Rot");
						break;
					case HUG_HAT_SWITCH:
				#ifdef __OpenBSD__
					case HUG_DPAD_UP:
				#endif
						jp.hats.push_back({usage, hats});
						jp.hid->hats().append( std::to_string(hats) + ".X" );
						jp.hid->hats().append( std::to_string(hats) + ".Y" );
						hats++;
					 	break;
				}

			} else if (HID_PAGE(item.usage) == HUP_BUTTON) {
				int usage = HID_USAGE(item.usage);
                jp.buttons.push_back({usage, buttons});
				jp.hid->buttons().append( std::to_string(buttons) );
				buttons++;
			}
	    }

	    hid_end_parse(hdata);

	    //printf("features: %i %i %i \n", buttons, hats, axes); fflush(NULL);

	    if (!buttons && !hats && !axes)
	        return false;
	    
	    fcntl(jp.fd, F_SETFL, O_NONBLOCK);

	#ifdef __NetBSD__
		while (read(fd, jp.report.data, jp.report.size) == jp.report.size) {}
	#endif

	    return true;
	}


	auto pollJoypad(std::vector<Hid::Device*>& devices) -> void {
		hid_item item;
    	hid_data* hdata;

		for (auto& jp : joypads) {

			for(auto& input : jp.hid->buttons().inputs)
				input.oldValue = input.value;
			
			for(auto& input : jp.hid->axes().inputs)
				input.oldValue = input.value;
			
			for(auto& input : jp.hid->hats().inputs)
				input.oldValue = input.value;

			while (read(jp.fd, jp.report.data, jp.report.size) == jp.report.size) {
				hdata = hid_start_parse(jp.desc, 1 << hid_input, jp.report.id);
				if (!hdata)
					continue;

				while (hid_get_item(hdata, &item) > 0) {
					if(item.kind != hid_input)
						continue;

					int code = HID_USAGE(item.usage);

					switch (HID_PAGE(item.usage)) {
						case HUP_GENERIC_DESKTOP: {
							if (auto input = findCode(jp.axes, code)) {
								int value = hid_get_data(jp.report.data, &item);
								int range = item.logical_maximum - item.logical_minimum;
								value = (value - item.logical_minimum) * 65535ll / range - 32767;
								jp.hid->axes().inputs[input->id].value = sclamp<16>(value);

							} else if (auto input = findCode(jp.hats, code)) {
								int value = hid_get_data(jp.report.data, &item);
								bool left = value == 5 || value == 6 || value == 7;
								bool right = value == 1 || value == 2 || value == 3;
								bool up = value == 0 || value == 1 || value == 7;
								bool down = value == 3 || value == 4 || value == 5;

								jp.hid->hats().inputs[input->id].value = left ? -32768 : (right ? +32767 : 0);
								jp.hid->hats().inputs[input->id + 1].value = up ? -32768 : (down ? +32767 : 0);
							}
						} break;

						case HUP_BUTTON: {		
							if (auto input = findCode(jp.buttons, code)) {
								int value = hid_get_data(jp.report.data, &item);
								jp.hid->buttons().inputs[input->id].value = value > 0;
							}
						} break;
					}
				}

				hid_end_parse(hdata);
			}

			devices.push_back(jp.hid);
		}
	}

	auto findCode(std::vector<JoypadInput>& inputs, int code ) -> JoypadInput* {
		for(auto& input : inputs) {
			if(input.code == code)
				return &input;
		}
		return nullptr;
	}

	auto free(Joypad& jp) -> void {
		if (jp.desc) {
			hid_dispose_report_desc(jp.desc);
			jp.desc = nullptr;
		}

		if (jp.report.data) {
			delete[] jp.report.data;
			jp.report.data = nullptr;
		}

		if (jp.fd) {
			close(jp.fd);
			jp.fd = -1;
		}

		if (jp.hid) {
			delete jp.hid;
			jp.hid = nullptr;
		}
	}

	auto term() -> void {

		for (auto& jp : joypads)
			free(jp);
			
		joypads.clear();
	}

	~Uhid() {
		term();
	}
};

}
