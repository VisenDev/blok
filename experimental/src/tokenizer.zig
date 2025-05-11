const std = @import("std");

pub const Token = union(enum) {
    end_of_input: void,
    symbol: []const u8,
    syntax: enum {
        block_open,
        block_close,
        lookup,
    },
    string_lit: []const u8,
    integer_lit: i64,
    float_lit: f64,
};

pub const Tokenizer = struct {
    input: []const u8,
    i: usize = 0,

    char: u8,
    token: Token = .{},
    line: usize = 1,
    column: usize = 1,

    pub fn init(input: []const u8) @This() {
        return .{
            .input = input,
        };
    }

    fn getChar(self: *@This()) ?u8 {
        if (self.input.len <= self.i) return null;

        const ch = self.input[self.i];
        //if (ch == '\n') {
        //    self.line += 1;
        //    self.column = 1;
        //} else {
        //    self.column += 1;
        //}

        self.ch = ch;
        self.i += 1;
        return ch;
    }

    fn getNumber(self: *@This()) !Token {
        //determine if input is a float or an integer
        var state: enum { integer, float } = .integer;
        var len: usize = 0;
        for (self.input[(self.i)..]) |ch| {
            len += 1;
            switch (ch) {
                '0'...'9', '_' => {
                    //continue
                },
                '.' => {
                    switch (state) {
                        .integer => {
                            state = .float;
                        },
                        .float => {
                            return error.tooManyPeriods;
                        },
                    }
                },
                else => {
                    //finish
                    break;
                },
            }
        }

        switch (state) {
            .integer => {
                const result: Token = .{
                    .integer_lit = try std.fmt.parseInt(
                        i64,
                        self.input[self.i .. self.i + len],
                    ),
                };
                self.i += len - 1;
                return result;
            },
            .float => {
                const result: Token = .{
                    .float_lit = try std.fmt.parseFloat(
                        f64,
                        self.input[self.i .. self.i + len],
                    ),
                };
                self.i += len - 1;
                return result;
            },
        }
    }

    fn getString(self: *@This(), a: ?std.mem.Allocator) !Token {
        _ = self; // autofix
        _ = a; // autofix

    }

    fn getSymbol(self: *@This(), a: ?std.mem.Allocator) !Token {
        _ = self; // autofix
        _ = a; // autofix

    }

    /// Allocates memory for token strings if an allocator is provided,
    /// otherwise will just return slices of the input.
    pub fn getToken(self: *@This(), a: ?std.mem.Allocator) !Token {
        if (self.getChar()) |ch| {
            switch (ch) {
                '0'...'9' => {
                    return self.getNumber();
                },
                '[', '(', '{', '<' => {
                    return .{ .syntax = .block_open };
                },
                ']', ')', '}', '>' => {
                    return .{ .syntax = .block_end };
                },
                ':' => return .{ .syntax = .lookup },
                '"' => {
                    return self.getString(a);
                },
                'a'...'z', 'A'...'Z', '-', '_', '?', '!' => {
                    return self.getSymbol(a);
                },
            }
        } else {
            return .{ .end_of_input = {} };
        }
    }
};
