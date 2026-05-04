#define TESLA_INIT_IMPL // If you have more than one file using the tesla header, only define this in the main one
#include <tesla.hpp>    // The Tesla Header

char toPrint[256];
HidSixAxisSensorHandle handles[4];
Result rc = 0;

Result hidsysSetAppletResourceUserId() {
	u64 aruid = appletGetAppletResourceUserId();
    return serviceDispatchIn(hidsysGetServiceSession(), 500, aruid);
}

class GuiTest : public tsl::Gui {
public:
	PadState pad;
	GuiTest(u8 arg1, u8 arg2, bool arg3) {
		padInitializeDefault(&pad);
	}

	// Called when this Gui gets loaded to create the UI
	// Allocate all elements on the heap. libtesla will make sure to clean them up when not needed anymore
	virtual tsl::elm::Element* createUI() override {
		// A OverlayFrame is the base element every overlay consists of. This will draw the default Title and Subtitle.
		// If you need more information in the header or want to change it's look, use a HeaderOverlayFrame.
		auto frame = new tsl::elm::OverlayFrame("ReverseNX-RT", APP_VERSION);

		// A list that can contain sub elements and handles scrolling
		auto list = new tsl::elm::List();
		
		list->addItem(new tsl::elm::CustomDrawer([](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
			renderer->drawString(toPrint, false, x, y+20, 20, renderer->a(0xFFFF));
	}), 150);

		// Add the list to the frame for it to be drawn
		frame->setContent(list);
        
		// Return the frame to have it become the top level element of this Gui
		return frame;
	}

	// Called once every frame to update values
	virtual void update() override {
		padUpdate(&pad);
        // Read from the correct sixaxis handle depending on the current input style
        HidSixAxisSensorState sixaxis = {0};
        u64 style_set = padGetStyleSet(&pad);
		size_t id = 0;
        if (style_set & HidNpadStyleTag_NpadHandheld)
            id = 0;
        else if (style_set & HidNpadStyleTag_NpadFullKey)
            id = 1;
        else if (style_set & HidNpadStyleTag_NpadJoyDual) {
            // For JoyDual, read from either the Left or Right Joy-Con depending on which is/are connected
            u64 attrib = padGetAttributes(&pad);
            if (attrib & HidNpadAttribute_IsLeftConnected)
                id = 2;
            else if (attrib & HidNpadAttribute_IsRightConnected)
                id = 3;
        }
		hidGetSixAxisSensorStates(handles[id], &sixaxis, 1);
		if (sixaxis.acceleration.x == 0.f && sixaxis.acceleration.y == 0.f && sixaxis.acceleration.z == -1.f) {
			if (R_SUCCEEDED(rc)) hidsysSetAppletResourceUserId();
			hidGetSixAxisSensorStates(handles[id], &sixaxis, 1);
		}

        snprintf(toPrint, sizeof(toPrint), "RC: 0x%x\nAcceleration:\nx=% .4f, y=% .4f, z=% .4f\nAngular velocity:\nx=% .4f, y=% .4f, z=% .4f\nAngle:\nx=% .4f, y=% .4f, z=% .4f\n", rc, sixaxis.acceleration.x, sixaxis.acceleration.y, sixaxis.acceleration.z, sixaxis.angular_velocity.x, sixaxis.angular_velocity.y, sixaxis.angular_velocity.z, sixaxis.angle.x, sixaxis.angle.y, sixaxis.angle.z);
	}

	// Called once every frame to handle inputs not handled by other UI elements
	virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
		if (keysDown & HidNpadButton_B) {
			tsl::goBack();
			tsl::goBack();
			return true;
		}
		return false;   // Return true here to singal the inputs have been consumed
	}
};

class Dummy : public tsl::Gui {
public:
	Dummy(u8 arg1, u8 arg2, bool arg3) {}

	// Called when this Gui gets loaded to create the UI
	// Allocate all elements on the heap. libtesla will make sure to clean them up when not needed anymore
	virtual tsl::elm::Element* createUI() override {
		auto frame = new tsl::elm::OverlayFrame("ReverseNX-RT", APP_VERSION);
		return frame;
	}

	// Called once every frame to handle inputs not handled by other UI elements
	virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
		tsl::changeTo<GuiTest>(0, 1, true);
		return true;   // Return true here to singal the inputs have been consumed
	}
};

class OverlayTest : public tsl::Overlay {
public:
	Service srv_hidbus;
	// libtesla already initialized fs, hid, pl, pmdmnt, hid:sys and set:sys
	virtual void initServices() override {
		rc = hidsysSetAppletResourceUserId();
		
		hidGetSixAxisSensorHandles(&handles[0], 1, HidNpadIdType_Handheld, HidNpadStyleTag_NpadHandheld);
    	hidGetSixAxisSensorHandles(&handles[1], 1, HidNpadIdType_No1,      HidNpadStyleTag_NpadFullKey);
    	hidGetSixAxisSensorHandles(&handles[2], 2, HidNpadIdType_No1,      HidNpadStyleTag_NpadJoyDual);
		hidStartSixAxisSensor(handles[0]);
		hidStartSixAxisSensor(handles[1]);
		hidStartSixAxisSensor(handles[2]);
		hidStartSixAxisSensor(handles[3]);
	}  // Called at the start to initialize all services necessary for this Overlay
	
	virtual void exitServices() override {
		hidStopSixAxisSensor(handles[0]);
		hidStopSixAxisSensor(handles[1]);
		hidStopSixAxisSensor(handles[2]);
		hidStopSixAxisSensor(handles[3]);
	}  // Callet at the end to clean up all services previously initialized

	virtual void onShow() override {}    // Called before overlay wants to change from invisible to visible state
	
	virtual void onHide() override {}    // Called before overlay wants to change from visible to invisible state

	virtual std::unique_ptr<tsl::Gui> loadInitialGui() override {
		return initially<Dummy>(1, 2, true);  // Initial Gui to load. It's possible to pass arguments to it's constructor like this
	}
};

int main(int argc, char **argv) {
    return tsl::loop<OverlayTest>(argc, argv);
}
