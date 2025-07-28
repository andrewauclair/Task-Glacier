package data;

import dialogs.AddTask;
import packets.*;
import taskglacier.MainFrame;

import javax.swing.*;
import java.io.*;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;

public class ServerConnection {
    private final DataInputStream input;
    private final DataOutputStream output;
    List<Packet> toSend = new ArrayList<>();

    public ServerConnection(DataInputStream input, DataOutputStream output) {
        this.input = input;
        this.output = output;
    }

    public void sendPacketWhenReady(Packet packet) {
        if (output != null) {
            sendPacket(packet);
        }
        else {
            toSend.add(packet);
        }
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
                    TaskInfo info = TaskInfo.parse(new DataInputStream(new ByteArrayInputStream(bytes)), packetLength);

                    SwingUtilities.invokeLater(() -> mainFrame.getTaskModel().receiveInfo(info));
                }
                else if (packetType == PacketType.REQUEST_CONFIGURATION_COMPLETE) {
                    SwingUtilities.invokeLater(MainFrame::restoreLayout);

                    SwingUtilities.invokeLater(() -> mainFrame.getTaskModel().configurationComplete());

                    for (Packet packet : toSend) {
                        sendPacket(packet);
                    }
                    toSend.clear();
                }
                else if (packetType == PacketType.BUGZILLA_INFO) {
                    BugzillaInfo info = BugzillaInfo.parse(new DataInputStream(new ByteArrayInputStream(bytes)), packetLength);

                    MainFrame.bugzillaInfo.put(info.name, info);
                }
                else if (packetType == PacketType.DAILY_REPORT) {
                    DailyReportMessage dailyReport = DailyReportMessage.parse(new DataInputStream(new ByteArrayInputStream(bytes)), packetLength);

                    SwingUtilities.invokeLater(() -> mainFrame.receivedDailyReport(dailyReport));
                }
                else if (packetType == PacketType.WEEKLY_REPORT) {
                    WeeklyReport report = WeeklyReport.parse(new DataInputStream(new ByteArrayInputStream(bytes)), packetLength);

                    SwingUtilities.invokeLater(() -> mainFrame.receivedWeeklyReport(report));
                }
                else if (packetType == PacketType.TIME_CATEGORIES_DATA) {
                    TimeCategoriesMessage message = TimeCategoriesMessage.parse(new DataInputStream(new ByteArrayInputStream(bytes)), packetLength);

                    mainFrame.getTimeData().processPacket(message);
                }
                else if (packetType == PacketType.FAILURE_RESPONSE) {
                    FailureResponse failure = FailureResponse.parse(new DataInputStream((new ByteArrayInputStream(bytes))), packetLength);

                    SwingUtilities.invokeLater(() -> {
                        if (AddTask.openInstance != null) {
                            AddTask.openInstance.failureResponse(failure.message);
                        }
                        else {
                             JOptionPane.showMessageDialog(mainFrame, failure.message, "Failure", JOptionPane.ERROR_MESSAGE);
                        }
                    });
                }
                else if (packetType == PacketType.SUCCESS_RESPONSE) {
                    SwingUtilities.invokeLater(() -> {
                        if (AddTask.openInstance != null) {
                            AddTask.openInstance.close();
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
                try (ByteArrayOutputStream output = new ByteArrayOutputStream(packet.size())) {
                    packet.writeToOutput(new DataOutputStream(output));
                }

                packet.writeToOutput(this.output);
                System.out.println("Sent packet with size: " + packet.size() + ", type: " + packet.type());
                return true;
            } catch (IOException e) {
                throw new RuntimeException(e);
            }
        }
        return false;
    }
}
