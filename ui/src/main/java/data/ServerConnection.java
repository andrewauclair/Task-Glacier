package data;

import dialogs.AddModifyTask;
import packets.FailureResponse;
import packets.Packet;
import packets.PacketType;
import packets.TaskInfo;
import taskglacier.MainFrame;

import javax.swing.*;
import java.io.ByteArrayInputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;

public class ServerConnection {
    private final DataInputStream input;
    private final DataOutputStream output;

    public ServerConnection(DataInputStream input, DataOutputStream output) {
        this.input = input;
        this.output = output;
    }

    // call this from a thread
    public void run(MainFrame mainFrame) {
        try {
            int packetLength;
            while ((packetLength = input.readInt()) != -1) {
                int expectedBytes = packetLength - 4;

                byte[] bytes = new byte[expectedBytes];

                int totalRead = 0;

                while (totalRead < expectedBytes) {
                    int read = input.read(bytes, totalRead, bytes.length - totalRead);
                    if (read == -1) {
                        totalRead = -1;
                        break;
                    }
                    totalRead += read;
                }

                if (totalRead == -1) {
                    break;
                }

                PacketType packetType = PacketType.valueOf(ByteBuffer.wrap(bytes, 0, 4).getInt());
                System.out.println("Received packet with length: " + packetLength + ", type: " + packetType);

                if (packetType == PacketType.TASK_INFO) {
                    TaskInfo info = TaskInfo.parse(new DataInputStream(new ByteArrayInputStream(bytes)));

                    SwingUtilities.invokeLater(() -> mainFrame.getTaskModel().receiveInfo(info));
                }
                else if (packetType == PacketType.REQUEST_CONFIGURATION_COMPLETE) {
                    SwingUtilities.invokeLater(MainFrame::restoreLayout);

                    SwingUtilities.invokeLater(() -> mainFrame.getTaskModel().configurationComplete());
                }
                else if (packetType == PacketType.FAILURE_RESPONSE) {
                    FailureResponse failure = FailureResponse.parse(new DataInputStream((new ByteArrayInputStream(bytes))));

                    SwingUtilities.invokeLater(() -> {
                        if (AddModifyTask.openInstance != null) {
                            AddModifyTask.openInstance.failureResponse(failure.message);
                        }
                        else {
                             JOptionPane.showMessageDialog(mainFrame, failure.message, "Failure", JOptionPane.ERROR_MESSAGE);
                        }
                    });
                }
                else if (packetType == PacketType.SUCCESS_RESPONSE) {
                    SwingUtilities.invokeLater(() -> {
                        if (AddModifyTask.openInstance != null) {
                            AddModifyTask.openInstance.close();
                        }
                    });
                }
            }
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    public void close() throws IOException {
        input.close();
        output.close();
    }

    public boolean sendPacket(Packet packet) {
        if (output != null) {
            try {
                packet.writeToOutput(output);
                return true;
            } catch (IOException e) {
//                throw new RuntimeException(e);
            }
        }
        return false;
    }
}
