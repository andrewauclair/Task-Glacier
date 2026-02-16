package dialogs;

import com.formdev.flatlaf.FlatClientProperties;
import com.formdev.flatlaf.extras.FlatSVGIcon;
import data.Standards;
import data.Task;
import packets.Basic;
import packets.CreateTask;
import packets.RequestID;
import taskglacier.MainFrame;
import util.DialogEscape;
import util.Icons;

import javax.swing.*;
import java.awt.*;
import java.awt.event.KeyAdapter;
import java.awt.event.KeyEvent;
import java.util.ArrayList;
import java.util.List;

import static taskglacier.MainFrame.mainFrame;

public class AddTask extends JDialog {
    private static final Dimension MAX_SIZE = new Dimension(600, 400);
    public static AddTask openInstance = null;

    public static List<Integer> activeRequests = new ArrayList<>();

    public AddTask(MainFrame mainFrame, Window parentWindow, int defaultParentID, boolean bulk) {
        super(parentWindow);
        openInstance = this;

        setModalityType(ModalityType.APPLICATION_MODAL);

        DialogEscape.addEscapeHandler(this);

        JButton add = new JButton("Add");
        add.setEnabled(false);

        // name, time tracking and project info
        // some of this info can be automatically filled
        JTextPane name = new JTextPane();
        JScrollPane scrollPane = new JScrollPane(name);

        setMaximumSize(MAX_SIZE);

        name.addKeyListener(new KeyAdapter() {
            @Override
            public void keyTyped(KeyEvent e) {
                SwingUtilities.invokeLater(() -> {
                    add.setEnabled(!name.getText().isEmpty());

                    Point center = new Point(getX() + getWidth() / 2, getY() + getHeight() / 2);

                    packRespectMax();

                    setLocation(center.x - getWidth() / 2, center.y - getHeight() / 2);

                    JScrollBar vertical = scrollPane.getVerticalScrollBar();
                    vertical.setValue(vertical.getMaximum());
                });
            }
        });

        JTextField parent = new JTextField();
        parent.setText(String.valueOf(defaultParentID));

        //JPanel flow = createFlow(bulk ? "Name(s): " : "Name: ", new JScrollPane(name));

        setTitle("Add Task");

        add.addActionListener(e -> {
            add.setEnabled(false);

            int parentID = Integer.parseInt(parent.getText());
            if (bulk) {
                String[] names = name.getText().split(System.lineSeparator());

                List<CreateTask> packets = new ArrayList<>();

                for (String s : names) {
                    if (s.isEmpty()) {
                        continue;
                    }
                    int requestID = RequestID.nextRequestID();
                    CreateTask create = new CreateTask(s, parentID, requestID);
                    packets.add(create);

                    activeRequests.add(requestID);
                }

                mainFrame.getConnection().sendPacket(Basic.BulkUpdateStart());

                for (CreateTask packet : packets) {
                    mainFrame.getConnection().sendPacket(packet);
                }

                mainFrame.getConnection().sendPacket(Basic.BulkUpdateFinish());
            }
            else {
                int requestID = RequestID.nextRequestID();
                CreateTask create = new CreateTask(name.getText(), parentID, requestID);

                activeRequests.add(requestID);

                mainFrame.getConnection().sendPacket(create);
            }
        });

        KeyAdapter enterAction = new KeyAdapter() {
            @Override
            public void keyReleased(KeyEvent e) {
                if (e.getKeyCode() == KeyEvent.VK_ENTER && (!bulk || e.isControlDown())) {
                    if (!bulk) {
                        // don't allow extra lines when adding a single task
                        String text = name.getText();
                        int beginIndex = text.indexOf(System.lineSeparator());
                        if (beginIndex != -1) {
                            name.setText(text.substring(0, beginIndex));
                        }
                    }
                    add.setEnabled(!name.getText().isEmpty());
                    add.doClick();
                }
            }
        };
        name.addKeyListener(enterAction);

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.weightx = 1.0;
        gbc.weighty = 1.0;
        gbc.fill = GridBagConstraints.BOTH;
        gbc.insets = new Insets(Standards.TOP_INSET, Standards.LEFT_INSET, Standards.BOTTOM_INSET, Standards.RIGHT_INSET);

        setLayout(new GridBagLayout());

        add(scrollPane, gbc);

        gbc.weightx = 0;
        gbc.weighty = 0;
        gbc.fill = GridBagConstraints.NONE;
        gbc.gridy++;

        JToolBar toolBar = new JToolBar();
        JButton search = new JButton(Icons.searchIcon);

        toolBar.add(search);

        parent.putClientProperty(FlatClientProperties.TEXT_FIELD_TRAILING_COMPONENT, toolBar);

        add(createFlow("Parent: ", parent), gbc);

        search.addActionListener(e -> {
            int taskID = 0;
            try {
                taskID = Integer.parseInt(parent.getText());
            }
            catch (NumberFormatException ignore) {
            }

            Task selectedParent = mainFrame.getTaskModel().getTask(taskID);
            TaskPicker picker = new TaskPicker(mainFrame, selectedParent);
            picker.setVisible(true);

            if (picker.task != null) {
                parent.setText(String.valueOf(picker.task.id));
                parent.setToolTipText(picker.task.name);
            }
        });

        gbc.gridy++;

        add(add, gbc);
        gbc.gridy++;

        pack();

        // center on the main frame
        setLocationRelativeTo(mainFrame);
    }

    public void failureResponse(String message) {
        JOptionPane.showMessageDialog(this, message, "Failure", JOptionPane.ERROR_MESSAGE);
        close();
    }

    public void close() {
        openInstance = null;
        AddTask.this.dispose();
    }

    JPanel createFlow(String name, JComponent comp) {
        JPanel panel = new JPanel(new FlowLayout());
        panel.add(new JLabel(name));
        panel.add(comp);
        return panel;
    }

    private void packRespectMax() {
        Container parent = this.getParent();
        if (parent != null) {
            parent.addNotify();
        }
        Dimension newSize = getPreferredSize();
        if (newSize.width > getMaximumSize().width) {
            newSize.width = getMaximumSize().width;
        }
        if (newSize.height > getMaximumSize().height) {
            newSize.height = getMaximumSize().height;
        }
        setSize(newSize);
    }
}
