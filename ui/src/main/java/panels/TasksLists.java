package panels;

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

public class TasksLists extends JPanel implements Dockable {

    private final JTable table;
    private final DefaultTableModel tableModel;
    private final String persistentID;
    private final String title;

    public TasksLists(MainFrame mainFrame, String persistentID, String title) {
        super(new BorderLayout());
        this.persistentID = persistentID;
        this.title = title;

        Docking.registerDockable(this);

        tableModel = new DefaultTableModel(new Object[]{"ID", "Name"}, 0);
        table = new JTable(tableModel);

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
            try {
                change.writeToStream(mainFrame.output);
            } catch (IOException ex) {
                throw new RuntimeException(ex);
            }
        });
        stop.addActionListener(e -> {
            int selectedRow = table.getSelectedRow();

            if (selectedRow == -1) {
                return;
            }

            TaskStateChange change = new TaskStateChange();
            change.packetType = PacketType.STOP_TASK;
            change.taskID = (int) tableModel.getValueAt(selectedRow, 0);
            try {
                change.writeToStream(mainFrame.output);
            } catch (IOException ex) {
                throw new RuntimeException(ex);
            }
        });
        finish.addActionListener(e -> {
            int selectedRow = table.getSelectedRow();

            if (selectedRow == -1) {
                return;
            }

            TaskStateChange change = new TaskStateChange();
            change.packetType = PacketType.FINISH_TASK;
            change.taskID = (int) tableModel.getValueAt(selectedRow, 0);
            try {
                change.writeToStream(mainFrame.output);
            } catch (IOException ex) {
                throw new RuntimeException(ex);
            }
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

    public void clear() {
        tableModel.setRowCount(0);
    }

    public void addTask(int id, String name) {
        tableModel.addRow(new Object[] { id, name });
        tableModel.fireTableRowsInserted(tableModel.getRowCount() - 1, tableModel.getRowCount());
    }

    public void taskModelUpdated() {

    }
}
