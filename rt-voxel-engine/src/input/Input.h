#pragma once

#include "action.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <unordered_map>
#include <fstream>
#include <string>
#include <sstream>
#include <iostream>

// keyboard buttons are defined from 32(Spacebar) and up, mouse buttons take first 8 'slots', 
// in between we have unused numbers, so we can define our input, for example mouse movement,
// or other devices input like gamepads
#define MOUSE_MOVE_UP (GLFW_MOUSE_BUTTON_LAST + 1)
#define MOUSE_MOVE_DOWN (GLFW_MOUSE_BUTTON_LAST + 2)
#define MOUSE_MOVE_LEFT (GLFW_MOUSE_BUTTON_LAST + 3)
#define MOUSE_MOVE_RIGHT (GLFW_MOUSE_BUTTON_LAST + 4)

#define CONSTANT_MOUSE_SENSITIVITY_MULTIPLIER 0.3
#define FAKE_MOUSE_INPUT_SPEED 720

class InputSystem 
{
public:
    static InputSystem& GetInstance() 
    {
        static InputSystem instance;
        return instance;
    }

    void Initialize(GLFWwindow* window, const std::string& configFilePath)
    {
        window_ = window;
        LoadKeyBindings(configFilePath);

        glfwSetKeyCallback(window_, KeyCallback);
        glfwSetMouseButtonCallback(window_, MouseButtonCallback);
        glfwSetCursorPosCallback(window_, CursorPositionCallback);
        glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) 
    {
        InputSystem& instance = GetInstance();
        if (action == GLFW_PRESS) 
            instance.keys_[key] = true;
        else if (action == GLFW_RELEASE) 
            instance.keys_[key] = false;
    }

    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) 
    {
        InputSystem& instance = GetInstance();
        if (action == GLFW_PRESS) 
            instance.keys_[button] = true;
        else if (action == GLFW_RELEASE) 
            instance.keys_[button] = false;
    }

    static void CursorPositionCallback(GLFWwindow* window, double xpos, double ypos) 
    {
        InputSystem& instance = GetInstance();
        instance.HandleMouseMovement(xpos, ypos);
    }

    void HandleMouseMovement(double xpos, double ypos) 
    {
        double deltaX = xpos - mouse_x_;
        double deltaY = ypos - mouse_y_;

        keys_[MOUSE_MOVE_LEFT] = deltaX < 0;
        keys_[MOUSE_MOVE_RIGHT] = deltaX > 0;
        keys_[MOUSE_MOVE_UP] = deltaY < 0;
        keys_[MOUSE_MOVE_DOWN] = deltaY > 0;

        mouse_x_ = xpos;
        mouse_y_ = ypos;
    }

    bool IsActionPressed(Action action)
    {
        auto it = action_key_map_.find(action);
        if (it != action_key_map_.end())
            return keys_[it->second];
        return false;
    }

    bool IsActionJustPressed(Action action)
    {
        auto it = action_key_map_.find(action);
        if (it != action_key_map_.end()) 
        {
            int key = it->second;
            if (keys_[key] && !previous_keys_[key]) 
            {
                previous_keys_[key] = true;
                return true;
            }
        }
        return false;
    }

    // if input bound to the action is non binary then return some value 
    // dependent and proportional to input for example: pixels traveled by cursor
    // else for binary input for example: pressed/released button
    // return some constant value close to something  achievable with non binary input in standard circumstances
    // if action was not found return 0.0
    double GetActionStrength(Action action, double delta_time)
    {
        auto it = action_key_map_.find(action);
        if (it != action_key_map_.end()) 
        {
            int key = it->second;

            if (key == MOUSE_MOVE_UP)
                return -mouse_y_ * CONSTANT_MOUSE_SENSITIVITY_MULTIPLIER;
            if (key == MOUSE_MOVE_DOWN)
                return mouse_y_ * CONSTANT_MOUSE_SENSITIVITY_MULTIPLIER;
            if (key == MOUSE_MOVE_LEFT)
                return -mouse_x_ * CONSTANT_MOUSE_SENSITIVITY_MULTIPLIER;
            if (key == MOUSE_MOVE_RIGHT)
                return mouse_x_ * CONSTANT_MOUSE_SENSITIVITY_MULTIPLIER;

            return FAKE_MOUSE_INPUT_SPEED * CONSTANT_MOUSE_SENSITIVITY_MULTIPLIER * delta_time;
        }

        return 0.0;
    }

    double GetMouseX() const 
    {
        return mouse_x_;
    }

    double GetMouseY() const 
    {
        return mouse_y_;
    }

    // it should be called AFTER handling input
    void Update() 
    {
        for (auto& kv : keys_)
            previous_keys_[kv.first] = kv.second;

        // reset cursor position for GLFW_CURSOR_DISABLED mode
        glfwSetCursorPos(window_, 0, 0);
        mouse_x_ = 0;
        mouse_y_ = 0;
    }

private:
    InputSystem() : mouse_x_(0.0), mouse_y_(0.0) {}
    ~InputSystem() {}

    // Disable copy and assignment
    InputSystem(const InputSystem&) = delete;
    InputSystem& operator=(const InputSystem&) = delete;

    void LoadKeyBindings(const std::string& configFilePath) 
    {
        std::ifstream file(configFilePath);
        if (!file.is_open()) 
        {
            std::cerr << "Failed to open config file: " << configFilePath << std::endl;
            return;
        }

        std::string line;
        while (std::getline(file, line)) 
        {
            std::istringstream iss(line);
            std::string actionStr, keyStr;
            if (!(iss >> actionStr >> keyStr))
                continue; // Invalid line

            int key;
            if (keyStr.length() == 1)
                key = CharToKeyCode(keyStr[0]);
            else 
                key = NameToKeyCode(keyStr);
            if (key == -1) 
            {
                std::cerr << "Invalid key: " << keyStr << " in line: " << line << std::endl;
                continue;
            }
            Action action = stringToAction(actionStr);
            action_key_map_[action] = key;
        }
    }

    int CharToKeyCode(char c) 
    {
        if (c >= 'A' && c <= 'Z') return GLFW_KEY_A + (c - 'A');
        if (c >= 'a' && c <= 'z') return GLFW_KEY_A + (c - 'a');
        if (c >= '0' && c <= '9') return GLFW_KEY_0 + (c - '0');
        switch (c) 
        {
        //case ' ': return GLFW_KEY_SPACE;
        case ',': return GLFW_KEY_COMMA;
        case '.': return GLFW_KEY_PERIOD;
        case '-': return GLFW_KEY_MINUS;
        case '+': return GLFW_KEY_EQUAL;
        case ';': return GLFW_KEY_SEMICOLON;
        case '\'': return GLFW_KEY_APOSTROPHE;
        case '/': return GLFW_KEY_SLASH;
        case '\\': return GLFW_KEY_BACKSLASH;
        case '[': return GLFW_KEY_LEFT_BRACKET;
        case ']': return GLFW_KEY_RIGHT_BRACKET;
        default: return -1; // Invalid key
        }
    }

    int NameToKeyCode(const std::string& name) 
    {
        if (name == "UP") return GLFW_KEY_UP;
        if (name == "DOWN") return GLFW_KEY_DOWN;
        if (name == "LEFT") return GLFW_KEY_LEFT;
        if (name == "RIGHT") return GLFW_KEY_RIGHT;
        if (name == "CAPS_LOCK") return GLFW_KEY_CAPS_LOCK;
        if (name == "LEFT_ALT") return GLFW_KEY_LEFT_ALT;
        if (name == "RIGHT_ALT") return GLFW_KEY_RIGHT_ALT;
        if (name == "LEFT_CONTROL") return GLFW_KEY_LEFT_CONTROL;
        if (name == "RIGHT_CONTROL") return GLFW_KEY_RIGHT_CONTROL;
        if (name == "LEFT_SHIFT") return GLFW_KEY_LEFT_SHIFT;
        if (name == "RIGHT_SHIFT") return GLFW_KEY_RIGHT_SHIFT;
        if (name == "ENTER") return GLFW_KEY_ENTER;
        if (name == "ESCAPE") return GLFW_KEY_ESCAPE;
        if (name == "SPACE") return GLFW_KEY_SPACE;
        if (name == "TAB") return GLFW_KEY_TAB;
        if (name == "BACKSPACE") return GLFW_KEY_BACKSPACE;
        if (name == "INSERT") return GLFW_KEY_INSERT;
        if (name == "DELETE") return GLFW_KEY_DELETE;
        if (name == "PAGE_UP") return GLFW_KEY_PAGE_UP;
        if (name == "PAGE_DOWN") return GLFW_KEY_PAGE_DOWN;
        if (name == "HOME") return GLFW_KEY_HOME;
        if (name == "END") return GLFW_KEY_END;
        if (name == "F1") return GLFW_KEY_F1;
        if (name == "F2") return GLFW_KEY_F2;
        if (name == "F3") return GLFW_KEY_F3;
        if (name == "F4") return GLFW_KEY_F4;
        if (name == "F5") return GLFW_KEY_F5;
        if (name == "F6") return GLFW_KEY_F6;
        if (name == "F7") return GLFW_KEY_F7;
        if (name == "F8") return GLFW_KEY_F8;
        if (name == "F9") return GLFW_KEY_F9;
        if (name == "F10") return GLFW_KEY_F10;
        if (name == "F11") return GLFW_KEY_F11;
        if (name == "F12") return GLFW_KEY_F12;
        // Mouse buttons
        if (name == "MOUSE_BUTTON_LEFT") return GLFW_MOUSE_BUTTON_LEFT;
        if (name == "MOUSE_BUTTON_RIGHT") return GLFW_MOUSE_BUTTON_RIGHT;
        if (name == "MOUSE_BUTTON_MIDDLE") return GLFW_MOUSE_BUTTON_MIDDLE;
        // Mouse movements
        if (name == "MOUSE_MOVE_UP") return MOUSE_MOVE_UP;
        if (name == "MOUSE_MOVE_DOWN") return MOUSE_MOVE_DOWN;
        if (name == "MOUSE_MOVE_LEFT") return MOUSE_MOVE_LEFT;
        if (name == "MOUSE_MOVE_RIGHT") return MOUSE_MOVE_RIGHT;
        // Add more special keys as needed
        return -1; // Invalid key name
    }

    GLFWwindow* window_ = nullptr;

    std::unordered_map<int, bool> keys_; // Key states
    std::unordered_map<int, bool> previous_keys_; // Previous key states
    std::unordered_map<Action, int> action_key_map_; // Mapping from actions to keys

    double mouse_x_, mouse_y_; // Current mouse position
};
