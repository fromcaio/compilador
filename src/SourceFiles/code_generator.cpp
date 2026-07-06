//
// Created by fc on 7/4/26.
//
#include "../HeaderFiles/code_generator.h"
#include <stdexcept>

// ==========================================
// REGISTRADORES
// ==========================================

std::string codegen_alloc_ireg()
{
    for (int i = 0; i < 7; ++i)
    {
        if (!codegen_ireg_used[i])
        {
            codegen_ireg_used[i] = true;
            return "t" + std::to_string(i);
        }
    }
    throw std::runtime_error("Sem registradores inteiros disponiveis (t0-t6 esgotados)");
}

std::string codegen_alloc_freg()
{
    for (int i = 0; i < 7; ++i)
    {
        if (!codegen_freg_used[i])
        {
            codegen_freg_used[i] = true;
            return "ft" + std::to_string(i);
        }
    }
    throw std::runtime_error("Sem registradores float disponiveis (ft0-ft6 esgotados)");
}

void codegen_free_reg(const std::string& reg)
{
    if (reg.empty()) return;
    // Inteiros: t0-t6
    if (reg.size() >= 2 && reg[0] == 't' && reg[1] != 'f')
    {
        int idx = std::stoi(reg.substr(1));
        if (idx >= 0 && idx < 7) codegen_ireg_used[idx] = false;
        return;
    }
    // Floats: ft0-ft6
    if (reg.size() >= 3 && reg[0] == 'f' && reg[1] == 't')
    {
        int idx = std::stoi(reg.substr(2));
        if (idx >= 0 && idx < 7) codegen_freg_used[idx] = false;
    }
}

// ==========================================
// EMISSÃO
// ==========================================

void codegen_emit(const std::string& line)
{
    if (!codegen_enabled) return;
    if (codegen_in_func)
        codegen_func_buffer << line << "\n";
    else if (codegen_main_out)
        *codegen_main_out << line << "\n";
}

void codegen_emit_global(const std::string& line)
{
    if (!codegen_enabled || !codegen_main_out) return;
    *codegen_main_out << line << "\n";
}

std::string codegen_new_label(const std::string& prefix)
{
    return "." + prefix + "_" + std::to_string(codegen_label_count++);
}

// ==========================================
// VARIÁVEIS GLOBAIS
// ==========================================

void codegen_global_var(const std::string& name, TokenType::Type type,
                        bool is_array, int array_size)
{
    if (!codegen_enabled) return;
    codegen_emit_global(".data");
    if (is_array)
    {
        int bytes = array_size > 0 ? array_size * 4 : 4;
        codegen_emit_global(name + ": .space " + std::to_string(bytes));
    }
    else if (type == TokenType::Type::TYPE_FLOAT)
    {
        codegen_emit_global(name + ": .float 0.0");
    }
    else
    {
        codegen_emit_global(name + ": .word 0");
    }
}

// ==========================================
// FUNÇÕES — PRÓLOGO / EPÍLOGO (com buffering)
// ==========================================

void codegen_func_begin(const std::string& name)
{
    if (!codegen_enabled) return;
    codegen_func_buffer.str("");
    codegen_func_buffer.clear();
    codegen_local_offsets.clear();
    codegen_frame_bytes = 0;
    for (int i = 0; i < 7; ++i) { codegen_ireg_used[i] = false; codegen_freg_used[i] = false; }
    codegen_in_func = true;
    codegen_current_func = name;
    codegen_func_end_label = codegen_new_label("func_end");

    codegen_emit_global("\n.text");
    codegen_emit_global(".globl " + name);
    codegen_emit_global(name + ":");
}

void codegen_func_end()
{
    if (!codegen_enabled) return;
    // frame = params + locais + 8 bytes (ra + fp salvos), alinhado a 16
    int frame = codegen_frame_bytes + 8;
    frame = (frame + 15) & ~15;

    int ra_off  = frame - 4;
    int fp_off  = frame - 8;

    // Prólogo (emitido diretamente no output principal, antes do buffer)
    codegen_emit_global("    addi sp, sp, -" + std::to_string(frame));
    codegen_emit_global("    sw   ra, " + std::to_string(ra_off) + "(sp)");
    codegen_emit_global("    sw   s0, " + std::to_string(fp_off) + "(sp)");
    codegen_emit_global("    addi s0, sp, " + std::to_string(frame));

    // Corpo (buffered)
    *codegen_main_out << codegen_func_buffer.str();

    // Epílogo
    codegen_emit_global(codegen_func_end_label + ":");
    codegen_emit_global("    lw   ra, " + std::to_string(ra_off) + "(sp)");
    codegen_emit_global("    lw   s0, " + std::to_string(fp_off) + "(sp)");
    codegen_emit_global("    addi sp, sp, " + std::to_string(frame));
    codegen_emit_global("    ret");

    codegen_in_func = false;
    codegen_func_buffer.str("");
    codegen_func_buffer.clear();
}

// ==========================================
// PARÂMETROS E LOCAIS
// ==========================================

void codegen_param(const std::string& name, TokenType::Type type, int index)
{
    if (!codegen_enabled) return;
    codegen_frame_bytes += 4;
    int offset = -(8 + codegen_frame_bytes);
    codegen_local_offsets[name] = offset;

    // Store argumento no slot local
    std::string areg = codegen_is_float_type(type)
        ? "fa" + std::to_string(index)
        : "a"  + std::to_string(index);
    std::string instr = codegen_is_float_type(type) ? "fsw" : "sw";
    codegen_emit("    " + instr + "  " + areg + ", " +
                 std::to_string(offset) + "(s0)");
}

void codegen_alloc_local(const std::string& name, TokenType::Type type)
{
    if (!codegen_enabled) return;
    (void)type;
    codegen_frame_bytes += 4;
    int offset = -(8 + codegen_frame_bytes);
    codegen_local_offsets[name] = offset;
    // Espaço reservado implicitamente pelo tamanho do frame; sem emit aqui
}

// ==========================================
// CARGA / STORE DE VARIÁVEIS
// ==========================================

ExprResult codegen_load_var(const std::string& name, TokenType::Type type)
{
    if (!codegen_enabled) return {type, ""};

    bool is_float = codegen_is_float_type(type);
    auto it = codegen_local_offsets.find(name);

    if (it != codegen_local_offsets.end())
    {
        // Variável local / parâmetro
        std::string reg = is_float ? codegen_alloc_freg() : codegen_alloc_ireg();
        std::string instr = is_float ? "flw" : "lw";
        codegen_emit("    " + instr + "  " + reg + ", " +
                     std::to_string(it->second) + "(s0)");
        return {type, reg};
    }
    else
    {
        // Variável global: carrega endereço e depois o valor
        std::string addr = codegen_alloc_ireg();
        codegen_emit("    la   " + addr + ", " + name);
        if (is_float)
        {
            std::string freg = codegen_alloc_freg();
            codegen_emit("    flw  " + freg + ", 0(" + addr + ")");
            codegen_free_reg(addr);
            return {type, freg};
        }
        else
        {
            codegen_emit("    lw   " + addr + ", 0(" + addr + ")");
            return {type, addr};
        }
    }
}

void codegen_store_var(const std::string& name, const std::string& reg, TokenType::Type type)
{
    if (!codegen_enabled || reg.empty()) return;

    bool is_float = codegen_is_float_type(type);
    auto it = codegen_local_offsets.find(name);

    if (it != codegen_local_offsets.end())
    {
        std::string instr = is_float ? "fsw" : "sw";
        codegen_emit("    " + instr + "  " + reg + ", " +
                     std::to_string(it->second) + "(s0)");
    }
    else
    {
        // Global: usa registrador temporário para o endereço
        std::string addr = codegen_alloc_ireg();
        codegen_emit("    la   " + addr + ", " + name);
        std::string instr = is_float ? "fsw" : "sw";
        codegen_emit("    " + instr + "  " + reg + ", 0(" + addr + ")");
        codegen_free_reg(addr);
    }
}
