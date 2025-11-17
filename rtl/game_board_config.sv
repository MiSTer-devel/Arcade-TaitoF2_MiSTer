import system_consts::*;

module game_board_config(
    input clk,
    input reset,
    input game_t game,

    output reg       cfg_360pri,
    output reg       cfg_360pri_high,
    output reg       cfg_110pcr,
    output reg       cfg_260dar,
    output reg       cfg_260dar_acc,
    output reg       cfg_190fmc,
    output reg [1:0] cfg_obj_extender,
    output reg       cfg_io_swap,
    output reg       cfg_tmp82c265,
    output reg       cfg_te7750,
    output reg       cfg_280grd,
    output reg       cfg_430grw,
    output reg       cfg_480scp,
    output reg       cfg_100scn,
    output reg       cfg_bpp15,
    output reg       cfg_bppmix,

    output reg [15:0] cfg_addr_rom,
    output reg [15:0] cfg_addr_rom1,
    output reg [15:0] cfg_addr_extra_rom,
    output reg [15:0] cfg_addr_work_ram,
    output reg [15:0] cfg_addr_share_ram,
    output reg [15:0] cfg_addr_screen0,
    output reg [15:0] cfg_addr_obj,
    output reg [15:0] cfg_addr_color,
    output reg [15:0] cfg_addr_io0,
    output reg [15:0] cfg_addr_io1,
    output reg [15:0] cfg_addr_sound,
    output reg [15:0] cfg_addr_extension,
    output reg [15:0] cfg_addr_priority,
    output reg [15:0] cfg_addr_cchip,

    output reg  [8:0] cfg_hofs_200obj,
    output reg  [8:0] cfg_vofs_200obj,
    output reg  [8:0] cfg_hofs_480scp,
    output reg  [8:0] cfg_vofs_480scp,
    output reg  [8:0] cfg_hofs_100scn0,
    output reg  [8:0] cfg_vofs_100scn0,
    output reg  [8:0] cfg_hofs_100scn1,
    output reg  [8:0] cfg_vofs_100scn1,
    output reg  [8:0] cfg_hofs_430grw,
    output reg  [8:0] cfg_vofs_430grw
);

// register these values to help with timing
//
always @(posedge clk) begin
    bit [16:0] c;

    case(game)
        //                      3  2 1 1  O 8 T I 2 4 4 1 B B
        //                      6  6 1 9  B 2 E O 8 3 8 0 P P
        //                      0  0 1 0  J C 7 S 0 0 0 0 P P
        //                      P  D P F  E 2 7 W G G S S 1 M
        //                      R  A C M  X 6 5 A R R C C 5 I
        //                      I  R R C  T 5 0 P D W P N   X
        GAME_CADASH:   c = 17'b00_00_1_0_00_0_0_0_0_0_0_0_0_0;
        default:       c = 17'b00_00_1_0_00_0_0_0_0_0_0_0_0_0;
    endcase

    { cfg_360pri_high, cfg_360pri, cfg_260dar_acc, cfg_260dar, cfg_110pcr, cfg_190fmc, cfg_obj_extender,
        cfg_tmp82c265, cfg_te7750, cfg_io_swap,
        cfg_280grd, cfg_430grw, cfg_480scp, cfg_100scn,
        cfg_bpp15, cfg_bppmix } <= c;
end

always_ff @(posedge clk) begin
    cfg_addr_rom       <= 16'hff00;
    cfg_addr_rom1      <= 16'hff00;
    cfg_addr_extra_rom <= 16'hff00;
    cfg_addr_work_ram  <= 16'hff00;
    cfg_addr_share_ram <= 16'hff00;
    cfg_addr_color     <= 16'hff00;
    cfg_addr_io0       <= 16'hff00;
    cfg_addr_io1       <= 16'hff00;
    cfg_addr_sound     <= 16'hff00;
    cfg_addr_screen0   <= 16'hff00;
    cfg_addr_obj       <= 16'hff00;
    cfg_addr_priority  <= 16'hff00;
    cfg_addr_extension <= 16'hff00;
    cfg_addr_cchip     <= 16'hff00;

    //cfg_addr_cchip     <= 16'b0;
    //cfg_addr_screen02  <= 16'b0;
    //cfg_addr_paddle_input  <= 16'b0;
    //cfg_addr_watchdog      <= 16'b0;
    //cfg_addr_dsw_coin_read <= 16'b0;
    //cfg_addr_mahjong_input <= 16'b0;
    //cfg_addr_gfxbank_sel   <= 16'b0;
    //cfg_addr_spritebank_sel<= 16'b0;
    //cfg_addr_rtc           <= 16'b0;
    //cfg_addr_extra_rom     <= 16'b0;

    case (game)
      GAME_CADASH: begin
        cfg_addr_rom      <= {8'h00, 8'hF8}; // 0x000000 - 0x07FFFF
        cfg_addr_work_ram      <= {8'h10, 8'hFF}; // 0x100000 - 0x10FFFF
        cfg_addr_share_ram     <= {8'h80, 8'hFF}; // 0x100000 - 0x10FFFF
        cfg_addr_color  <= {8'hA0, 8'hFF}; // 0xa00000 - 0xa00007 (TC0110PCR)
        cfg_addr_io0       <= {8'h90, 8'hFF}; // 0x900000 - 0x90000F (TC0220IOC)
        cfg_addr_screen0  <= {8'hC0, 8'hF0}; // 0x800000 - 0x80FFFF (TC0100SCN RAM)
        cfg_addr_sound     <= {8'h0C, 8'hFF};
      end

      default: begin
      end

    endcase
end

always_ff @(posedge clk) begin
    if (reset) begin
        cfg_hofs_200obj  <= 312; cfg_vofs_200obj  <= 237;
        cfg_hofs_100scn0 <= 397; cfg_vofs_100scn0 <= 253;
        cfg_hofs_100scn1 <= 397; cfg_vofs_100scn1 <= 253;
        cfg_hofs_430grw  <= 323; cfg_vofs_430grw  <= 224;

        case (game)
            default:        begin cfg_hofs_480scp <= 322;      cfg_vofs_480scp <= 224;      end
        endcase
    end
end

endmodule

