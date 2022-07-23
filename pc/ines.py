import kaitaistruct
from kaitaistruct import KaitaiStruct, KaitaiStream, BytesIO
from enum import Enum


if getattr(kaitaistruct, 'API_VERSION', (0, 9)) < (0, 9):
    raise Exception("Incompatible Kaitai Struct Python API: 0.9 or later is required, but you have %s" % (kaitaistruct.__version__))

class Ines(KaitaiStruct):
    """
    .. seealso::
       Source - https://wiki.nesdev.com/w/index.php/INES
    """
    def __init__(self, _io, _parent=None, _root=None):
        self._io = _io
        self._parent = _parent
        self._root = _root if _root else self
        self._read()

    def _read(self):
        self._raw_header = self._io.read_bytes(16)
        _io__raw_header = KaitaiStream(BytesIO(self._raw_header))
        self.header = Ines.Header(_io__raw_header, self, self._root)
        if self.header.f6.trainer:
            self.trainer = self._io.read_bytes(512)

        self.prg_rom = self._io.read_bytes((self.header.len_prg_rom * 16384))
        self.chr_rom = self._io.read_bytes((self.header.len_chr_rom * 8192))
        if self.header.f7.playchoice10:
            self.playchoice10 = Ines.Playchoice10(self._io, self, self._root)

        if not (self._io.is_eof()):
            self.title = (self._io.read_bytes_full()).decode(u"ASCII")


    class Header(KaitaiStruct):
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._read()

        def _read(self):
            self.magic = self._io.read_bytes(4)
            if not self.magic == b"\x4E\x45\x53\x1A":
                raise kaitaistruct.ValidationNotEqualError(b"\x4E\x45\x53\x1A", self.magic, self._io, u"/types/header/seq/0")
            self.len_prg_rom = self._io.read_u1()
            self.len_chr_rom = self._io.read_u1()
            self._raw_f6 = self._io.read_bytes(1)
            _io__raw_f6 = KaitaiStream(BytesIO(self._raw_f6))
            self.f6 = Ines.Header.F6(_io__raw_f6, self, self._root)
            self._raw_f7 = self._io.read_bytes(1)
            _io__raw_f7 = KaitaiStream(BytesIO(self._raw_f7))
            self.f7 = Ines.Header.F7(_io__raw_f7, self, self._root)
            self.len_prg_ram = self._io.read_u1()
            self._raw_f9 = self._io.read_bytes(1)
            _io__raw_f9 = KaitaiStream(BytesIO(self._raw_f9))
            self.f9 = Ines.Header.F9(_io__raw_f9, self, self._root)
            self._raw_f10 = self._io.read_bytes(1)
            _io__raw_f10 = KaitaiStream(BytesIO(self._raw_f10))
            self.f10 = Ines.Header.F10(_io__raw_f10, self, self._root)
            self.reserved = self._io.read_bytes(5)
            if not self.reserved == b"\x00\x00\x00\x00\x00":
                raise kaitaistruct.ValidationNotEqualError(b"\x00\x00\x00\x00\x00", self.reserved, self._io, u"/types/header/seq/8")

        class F6(KaitaiStruct):
            """
            .. seealso::
               Source - https://wiki.nesdev.com/w/index.php/INES#Flags_6
            """

            class Mirroring(Enum):
                horizontal = 0
                vertical = 1
            def __init__(self, _io, _parent=None, _root=None):
                self._io = _io
                self._parent = _parent
                self._root = _root if _root else self
                self._read()

            def _read(self):
                self.lower_mapper = self._io.read_bits_int_be(4)
                self.four_screen = self._io.read_bits_int_be(1) != 0
                self.trainer = self._io.read_bits_int_be(1) != 0
                self.has_battery_ram = self._io.read_bits_int_be(1) != 0
                self.mirroring = KaitaiStream.resolve_enum(Ines.Header.F6.Mirroring, self._io.read_bits_int_be(1))


        class F7(KaitaiStruct):
            """
            .. seealso::
               Source - https://wiki.nesdev.com/w/index.php/INES#Flags_7
            """
            def __init__(self, _io, _parent=None, _root=None):
                self._io = _io
                self._parent = _parent
                self._root = _root if _root else self
                self._read()

            def _read(self):
                self.upper_mapper = self._io.read_bits_int_be(4)
                self.format = self._io.read_bits_int_be(2)
                self.playchoice10 = self._io.read_bits_int_be(1) != 0
                self.vs_unisystem = self._io.read_bits_int_be(1) != 0


        class F9(KaitaiStruct):
            """
            .. seealso::
               Source - https://wiki.nesdev.com/w/index.php/INES#Flags_9
            """

            class TvSystem(Enum):
                ntsc = 0
                pal = 1
            def __init__(self, _io, _parent=None, _root=None):
                self._io = _io
                self._parent = _parent
                self._root = _root if _root else self
                self._read()

            def _read(self):
                self.reserved = self._io.read_bits_int_be(7)
                self.tv_system = KaitaiStream.resolve_enum(Ines.Header.F9.TvSystem, self._io.read_bits_int_be(1))


        class F10(KaitaiStruct):
            """
            .. seealso::
               Source - https://wiki.nesdev.com/w/index.php/INES#Flags_10
            """

            class TvSystem(Enum):
                ntsc = 0
                dual1 = 1
                pal = 2
                dual2 = 3
            def __init__(self, _io, _parent=None, _root=None):
                self._io = _io
                self._parent = _parent
                self._root = _root if _root else self
                self._read()

            def _read(self):
                self.reserved1 = self._io.read_bits_int_be(2)
                self.bus_conflict = self._io.read_bits_int_be(1) != 0
                self.prg_ram = self._io.read_bits_int_be(1) != 0
                self.reserved2 = self._io.read_bits_int_be(2)
                self.tv_system = KaitaiStream.resolve_enum(Ines.Header.F10.TvSystem, self._io.read_bits_int_be(2))


        @property
        def mapper(self):
            """
            .. seealso::
               Source - https://wiki.nesdev.com/w/index.php/Mapper
            """
            if hasattr(self, '_m_mapper'):
                return self._m_mapper

            self._m_mapper = (self.f6.lower_mapper | (self.f7.upper_mapper << 4))
            return getattr(self, '_m_mapper', None)


    class Playchoice10(KaitaiStruct):
        """
        .. seealso::
           Source - http://wiki.nesdev.com/w/index.php/PC10_ROM-Images
        """
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._read()

        def _read(self):
            self.inst_rom = self._io.read_bytes(8192)
            self.prom = Ines.Playchoice10.Prom(self._io, self, self._root)

        class Prom(KaitaiStruct):
            def __init__(self, _io, _parent=None, _root=None):
                self._io = _io
                self._parent = _parent
                self._root = _root if _root else self
                self._read()

            def _read(self):
                self.data = self._io.read_bytes(16)
                self.counter_out = self._io.read_bytes(16)