// N-Api wrapper
// wraps the compiler,cpu and memory module in a Node-Api class and exposes methods such as compile,run , step etc.

#include <napi.h>

#include "compiler/Assembler.h"
#include "cpu/Cpu8085.h"
#include "memory/Memory.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>

class SimulatorWrapper final : public Napi::ObjectWrap<SimulatorWrapper> {
public:
    static Napi::Function DefineClass(Napi::Env env) {
        return ObjectWrap::DefineClass(env, "SimulatorWrapper", {
            InstanceMethod("Compile", &SimulatorWrapper::Compile),
            InstanceMethod("Run", &SimulatorWrapper::Run),
            InstanceMethod("Step", &SimulatorWrapper::Step),
            InstanceMethod("Reset", &SimulatorWrapper::Reset),
            InstanceMethod("Pause", &SimulatorWrapper::Pause),
            InstanceMethod("AddBreakpoint", &SimulatorWrapper::AddBreakpoint),
            InstanceMethod("InspectMemory", &SimulatorWrapper::InspectMemory),
            InstanceMethod("InjectMemory", &SimulatorWrapper::InjectMemory),
            InstanceMethod("SetRegister", &SimulatorWrapper::SetRegister),
            InstanceMethod("GetRegisters", &SimulatorWrapper::GetRegisters),
        });
    }

    explicit SimulatorWrapper(const Napi::CallbackInfo& info)
        : Napi::ObjectWrap<SimulatorWrapper>(info), cpu_(memory_) {}

    ~SimulatorWrapper() override {
        PauseWorker();
    }

private:
    Napi::Value Compile(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        if (info.Length() < 1 || !info[0].IsString()) {
            throw Napi::TypeError::New(env, "Compile expects an assembly string");
        }
        try {
            const auto code = assembler_.Compile(info[0].As<Napi::String>().Utf8Value());
            auto result = Napi::Buffer<uint8_t>::New(env, code.size());
            std::copy(code.begin(), code.end(), result.Data());
            return result;
        } catch (const std::exception& error) {
            throw Napi::Error::New(env, error.what());
        }
    }

    Napi::Value Run(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        if (info.Length() < 1 || !info[0].IsFunction()) {
            throw Napi::TypeError::New(env, "Run expects a callback");
        }
        PauseWorker();
        stopRequested_.store(false);
        auto callback = info[0].As<Napi::Function>();
        auto tsfn = Napi::ThreadSafeFunction::New(env, callback, "8085-run", 0, 1);
        worker_ = std::thread([this, tsfn]() mutable {
            while (!stopRequested_.load()) {
                try {
                    const auto state = cpu_.Step();
                    tsfn.BlockingCall(new simulator::CpuState(state),
                        [this](Napi::Env env, Napi::Function callback, simulator::CpuState* state) {
                            callback.Call({ToObject(env, *state)});
                            delete state;
                        });
                    if (state.halted) break;
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                } catch (...) {
                    break;
                }
            }
            tsfn.Release();
        });
        return env.Undefined();
    }

    Napi::Value Step(const Napi::CallbackInfo& info) {
        return ToObject(info.Env(), cpu_.Step());
    }

    Napi::Value Reset(const Napi::CallbackInfo& info) {
        PauseWorker();
        memory_.Reset();
        cpu_.Reset();
        return info.Env().Undefined();
    }

    Napi::Value Pause(const Napi::CallbackInfo& info) {
        PauseWorker();
        return info.Env().Undefined();
    }

    Napi::Value AddBreakpoint(const Napi::CallbackInfo& info) {
        RequireArguments(info, 1, "AddBreakpoint expects an address");
        cpu_.AddBreakpoint(ToAddress(info[0]));
        return info.Env().Undefined();
    }

    Napi::Value InspectMemory(const Napi::CallbackInfo& info) {
        RequireArguments(info, 2, "InspectMemory expects address and length");
        const auto bytes = memory_.ReadRange(ToAddress(info[0]), ToLength(info[1]));
        auto result = Napi::Buffer<uint8_t>::New(info.Env(), bytes.size());
        std::copy(bytes.begin(), bytes.end(), result.Data());
        return result;
    }

    Napi::Value InjectMemory(const Napi::CallbackInfo& info) {
        RequireArguments(info, 2, "InjectMemory expects address and Buffer");
        if (!info[1].IsBuffer()) throw Napi::TypeError::New(info.Env(), "Second argument must be a Buffer");
        const auto buffer = info[1].As<Napi::Buffer<uint8_t>>();
        memory_.WriteRange(ToAddress(info[0]), buffer.Data(), buffer.Length());
        return info.Env().Undefined();
    }

    Napi::Value SetRegister(const Napi::CallbackInfo& info) {
        RequireArguments(info, 2, "SetRegister expects name and value");
        if (!info[0].IsString() || !info[1].IsNumber()) throw Napi::TypeError::New(info.Env(), "Invalid register arguments");
        cpu_.SetRegister(info[0].As<Napi::String>().Utf8Value(), info[1].As<Napi::Number>().Uint32Value());
        return info.Env().Undefined();
    }

    Napi::Value GetRegisters(const Napi::CallbackInfo& info) {
        return ToObject(info.Env(), cpu_.GetState());
    }

    static void RequireArguments(const Napi::CallbackInfo& info, std::size_t count, const char* message) {
        if (info.Length() < count) throw Napi::TypeError::New(info.Env(), message);
    }

    static uint16_t ToAddress(const Napi::Value& value) {
        if (!value.IsNumber() || value.As<Napi::Number>().Uint32Value() > 0xFFFF) throw Napi::RangeError::New(value.Env(), "Address must be 0..65535");
        return static_cast<uint16_t>(value.As<Napi::Number>().Uint32Value());
    }

    static std::size_t ToLength(const Napi::Value& value) {
        if (!value.IsNumber()) throw Napi::TypeError::New(value.Env(), "Length must be a number");
        return value.As<Napi::Number>().Uint32Value();
    }

    static Napi::Object ToObject(Napi::Env env, const simulator::CpuState& state) {
        auto object = Napi::Object::New(env);
        object.Set("a", state.a); object.Set("b", state.b); object.Set("c", state.c);
        object.Set("d", state.d); object.Set("e", state.e); object.Set("h", state.h); object.Set("l", state.l);
        object.Set("pc", state.pc); object.Set("sp", state.sp); object.Set("sign", state.sign); object.Set("zero", state.zero);
        object.Set("auxiliaryCarry", state.auxiliaryCarry); object.Set("parity", state.parity); object.Set("carry", state.carry);
        object.Set("halted", state.halted);
        return object;
    }

    void PauseWorker() {
        stopRequested_.store(true);
        if (worker_.joinable()) worker_.join();
    }

    simulator::Memory memory_;
    simulator::Assembler assembler_;
    simulator::Cpu8085 cpu_;
    std::atomic<bool> stopRequested_{false};
    std::thread worker_;
};

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    exports.Set("SimulatorWrapper", SimulatorWrapper::DefineClass(env));
    return exports;
}

NODE_API_MODULE(8085_simulator, Init)