package panels;

import data.TaskModel;
import data.TaskState;
import io.github.andrewauclair.moderndocking.Dockable;
import io.github.andrewauclair.moderndocking.app.Docking;
import packets.PacketType;
import packets.TaskStateChange;
import taskglacier.MainFrame;

import javax.swing.*;
import javax.swing.table.DefaultTableModel;
import java.awt.*;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.io.IOException;

public class TasksLists extends JPanel implements Dockable, TaskModel.Listener {

    private final JTable table;
    private final DefaultTableModel tableModel;
    private final MainFrame mainFrame;
    private final String persistentID;
    private final String title;

    public TasksLists(MainFrame mainFrame, String persistentID, String title) {
        super(new BorderLayout());
        this.mainFrame = mainFrame;
        this.persistentID = persistentID;
        this.title = title;

        Docking.registerDockable(this);
        mainFrame.getTaskModel().addListener(this);

        tableModel = new DefaultTableModel(new Object[]{"ID", "Name"}, 0) {
            @Override
            public boolean isCellEditable(int row, int column) {
                return false;
            }
        };
        table = new JTable(tableModel) {

        };

        JPopupMenu contextMenu = new JPopupMenu();
        JMenuItem start = new JMenuItem("Start");
        JMenuItem stop = new JMenuItem("Stop");
        JMenuItem finish = new JMenuItem("Finish");

        start.addActionListener(e -> {
            int selectedRow = table.getSelectedRow();

            if (selectedRow == -1) {
                return;
            }

            TaskStateChange change = new TaskStateChange();
            change.packetType = PacketType.START_TASK;
            change.taskID = (int) tableModel.getValueAt(selectedRow, 0);
            mainFrame.getConnection().sendPacket(change);
        });
        stop.addActionListener(e -> {
            int selectedRow = table.getSelectedRow();

            if (selectedRow == -1) {
                return;
            }

            TaskStateChange change = new TaskStateChange();
            change.packetType = PacketType.STOP_TASK;
            change.taskID = (int) tableModel.getValueAt(selectedRow, 0);
            mainFrame.getConnection().sendPacket(change);
        });
        finish.addActionListener(e -> {
            int selectedRow = table.getSelectedRow();

            if (selectedRow == -1) {
                return;
            }

            TaskStateChange change = new TaskStateChange();
            change.packetType = PacketType.FINISH_TASK;
            change.taskID = (int) tableModel.getValueAt(selectedRow, 0);
            mainFrame.getConnection().sendPacket(change);
        });

        contextMenu.add(start);
        contextMenu.add(stop);
        contextMenu.add(finish);

        table.addMouseListener(new MouseAdapter() {
            @Override
            public void mouseClicked(MouseEvent e) {
                if (SwingUtilities.isRightMouseButton(e)) {
                    contextMenu.show(table, e.getX(), e.getY());
                }
                else if (e.getClickCount() == 2 && SwingUtilities.isLeftMouseButton(e)) {
                    start.doClick();
                }
            }
        });

        add(new JScrollPane(table));
    }

    @Override
    public String getPersistentID() {
        return persistentID;
    }

    @Override
    public String getTabText() {
        return title;
    }

    @Override
    public boolean isWrappableInScrollpane() {
        return false;
    }

    @Override
    public void cleared() {
        tableModel.setRowCount(0);
    }

    @Override
    public void newTask(int taskID) {
        String name = mainFrame.getTaskModel().getTaskName(taskID);
        tableModel.addRow(new Object[] { taskID, name });
        tableModel.fireTableRowsInserted(tableModel.getRowCount() - 1, tableModel.getRowCount());
    }

    @Override
    public void updatedTask(int taskID) {
        if (mainFrame.getTaskModel().getTaskState(taskID) == TaskState.FINISHED) {
            for (int i = 0; i < tableModel.getRowCount(); i++) {
                if ((int) tableModel.getValueAt(i, 0) == taskID) {
                    tableModel.removeRow(i);
                    break;
                }
            }
        }
    }
}
