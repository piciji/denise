
#include "uci.h"
#include "../../../tools/buffer.h"
#include "../../../tools/memchangetracker.h"

namespace LIBC64 {

auto Uci::reset() -> void {
    enabled = false;
    commandQueue.clear();
    dirFiles.clear();

    packetPos = 0;
    filePos = 0;
    statPos = 0;
    openedIdent = ~0;

    fileData.reset();
}

auto Uci::read(uint8_t addr) -> uint8_t {
    uint8_t val = 0xff;

    switch (addr) {
        case 0x1c: // status
            if (!enabled)
                break;

            val = 0;
            switch (command) {
                case Command::GetPath:
                    if (packetPos == 0)
                        val = DATA_AV | DATA_LAST;
                    else
                        val = STAT_AV | DATA_LAST;
                    break;
                case Command::ChangeDir:
                    val = STAT_AV | DATA_LAST;
                    break;
                case Command::DeleteFile:
                    val = STAT_AV | DATA_LAST;
                    break;
                case Command::OpenFile: {
                    if (packetPos == 0) {
                        val = STAT_AV | DATA_LAST;
                        if (commandQueue.size() > 3) {
                            std::string str(commandQueue.begin() + 3, commandQueue.end());
                            openedIdent = findFileIdent(str);
                            fileData = interface->getFileFromArchive( media, openedIdent );
                        }
                    }
                } break;
                case Command::CloseFile:
                    val = STAT_AV | DATA_LAST;
                    if (statPos == 0) {
                        fileData.reset();
                        openedIdent = ~0;
                    }
                    break;
                case Command::OpenDir:
                    val = STAT_AV | DATA_LAST;
                    break;
                case Command::ReadDir:
                    if (dirFiles.empty())
                        break;

                    val = DATA_AV;
                    val |= dirFiles.size() > 1 ? DATA_MORE : DATA_LAST;

                    if (packetPos > dirFiles[filePos].second.size()) {
                        packetPos = 0;

                        //fprintf( stdout, "file: %s\n", dirFiles[filePos].second.c_str() );

                        filePos++;
                        val = filePos < dirFiles.size() ? DATA_MORE : DATA_LAST;

                        if (filePos >= dirFiles.size()) {
                            val = 0;
                        }
                    }
                    break;
                case Command::LoadReu:
                    val = STAT_AV | DATA_LAST;
                    if (commandQueue.size() == 10) {
                        auto reuAddr = Emulator::copyBufferToInt<uint32_t>( &commandQueue[2] );
                        auto reuLength = Emulator::copyBufferToInt<uint32_t>( &commandQueue[6] );
                        //fprintf( stdout, "REU addr %x, length %i\n", reuAddr, reuLength );

                        if (targetPtr && fileData.ptr) {
                            unsigned transferSize = std::min(fileData.size, reuLength);
                            if (reuAddr < targetSize) {
                                if ((reuAddr + transferSize) > targetSize)
                                    transferSize = targetSize - reuAddr;

                                auto* memChange = expansioPort->memChange;

                                if (memChange) { // history support
                                    for (unsigned i = reuAddr; i < (reuAddr + transferSize); i++) {
                                        memChange->remember(i);
                                    }
                                }

                                std::memcpy(targetPtr + reuAddr, fileData.ptr, transferSize );
                            }
                        }
                    }
                    break;

                default:
                    break;
            } break;

        case 0x1d: // identification
            if (media && interface->isArchivedMedia( media )) {
                val = 0xc9;
                dirFiles = interface->getFileList( media );
                enabled = true;
                fprintf( stderr, "UCI: Enable\n");
            }
            break;

        case 0x1e: // data packet
            if (!enabled)
                break;

            if (command == Command::GetPath) {
                val = '/';
            } else if (command == Command::ReadDir) {
                if (packetPos == 0)
                    val = 0x0;
                else {
                    const std::string& _f = dirFiles[filePos].second;
                    val = _f[packetPos-1];
                }
            }

            packetPos++;
            break;

        case 0x1f: { // status packet
            if (!enabled)
                break;
            constexpr static uint8_t ok[] = { '0','0',',','O','K' };

            if ((command == Command::OpenFile || command == Command::LoadReu) && !fileData.ptr) {
                val = 1;
                command = Command::None;
            } else if (command != Command::None) {
                val = ok[statPos++];
                if (statPos == 5)
                    command = Command::None;
            } else
                val = 0;

        } break;

        default:
            break;
    }

    // if (enabled)
    //   fprintf( stdout, "read: %x %x \n", addr, val  );

    return val;
}

auto Uci::write(uint8_t addr, uint8_t value) -> void {
    if (!enabled)
        return;

    // fprintf( stdout, "write: %x %x\n", addr, value  );

    switch (addr) {
        case 0x1c: { // control
            bool clearCommand = false;

            if (value & 1) { // push cmd
                packetPos = 0;
                statPos = 0;
                filePos = 0;
                command = Command::None;
                if (commandQueue.size() >= 2) {
                    if (commandQueue[0] == 1 || commandQueue[0] == 2) {
                        switch (commandQueue[1]) {
                            case 0x02: {
                                if (commandQueue.size() > 3) {
                                    std::string str(commandQueue.begin() + 3, commandQueue.end());
                                    fprintf( stdout, "UCI: Open File %x %s\n", commandQueue[2], str.c_str());
                                } else
                                    fprintf( stderr, "UCI: Open File ?\n");
                                command = Command::OpenFile;
                            } break;
                            case 0x03:
                                fprintf( stdout, "UCI: Close File\n");
                                command = Command::CloseFile;
                                break;
                            case 0x09: {
                                std::string str(commandQueue.begin() + 2, commandQueue.end());
                                fprintf( stdout, "UCI: Delete File %s\n", str.c_str());
                                command = Command::DeleteFile;
                            } break;
                            case 0x11: {
                                std::string str(commandQueue.begin() + 2, commandQueue.end());
                                fprintf( stdout, "UCI: Change Dir %s\n", str.c_str());
                                command = Command::ChangeDir;
                            } break;
                            case 0x12:
                                fprintf( stdout, "UCI: Get Path\n");
                                command = Command::GetPath;
                                break;
                            case 0x13:
                                fprintf( stdout, "UCI: Open Dir\n");
                                command = Command::OpenDir;
                                break;
                            case 0x14:
                                fprintf( stdout, "UCI: Read Dir\n");
                                command = Command::ReadDir;
                                break;
                            case 0x21:
                                fprintf( stdout, "UCI: Load Reu\n");
                                command = Command::LoadReu;
                                break;
                            default:
                                fprintf( stderr, "Unknown UCI command %x", commandQueue[1] );
                                break;
                        }
                    }
                }
            }

            if (value & 2) { // accepted
                clearCommand = true;
                if (command == Command::ReadDir) {
                    if (filePos < dirFiles.size())
                        clearCommand = false;
                }
            }

            if (value & 4) // abort
                clearCommand = true;

            if (value & 0xe8) {
                fprintf( stderr, "Unknown UCI control %x\n", value);
            }

            if (clearCommand) {
                command = Command::None;
                commandQueue.clear();
                packetPos = 0;
                statPos = 0;
            }
        } break;

        case 0x1d:
            if (commandQueue.size() < QUEUE_MAX_SIZE)
                commandQueue.push_back( value );
            break;

        default:
            break;
    }
}

auto Uci::setMedia(Emulator::Interface::Media* media) -> void {
    this->media = media;
    if (!media) {
        dirFiles.clear();
        fileData.reset();
        enabled = false;
    }
}

auto Uci::setTarget(uint8_t* _data, unsigned _size) -> void {
    targetPtr = _data;
    targetSize = _size;
}

auto Uci::findFileIdent(const std::string& fileName) -> unsigned {
    for (auto& _f : dirFiles) {
        if (_f.second == fileName)
            return _f.first;
    }
    return ~0;
}

auto Uci::serialize(Emulator::Serializer& s) -> void {
    s.integer( enabled );
    s.integer( packetPos );
    s.integer( statPos );
    s.integer( filePos );
    s.integer( openedIdent );
    if (s.mode() == Emulator::Serializer::Mode::Size)
        s.bufferPtr( QUEUE_MAX_SIZE + 4 ); // reserve some space
    else
        s.vector( commandQueue );

    if (!s.memUsage() && (s.mode() == Emulator::Serializer::Mode::Load) && media ) {
        if (dirFiles.empty())
            dirFiles = interface->getFileList( media );

        if (openedIdent != ~0)
            fileData = interface->getFileFromArchive( media, openedIdent );
        else
            fileData.reset();
    }
}

}
