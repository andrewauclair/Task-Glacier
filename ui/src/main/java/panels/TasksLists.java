package panels;

import data.Task;
import data.TaskModel;
import data.TaskState;
import io.github.andrewauclair.moderndocking.Dockable;
import io.github.andrewauclair.moderndocking.app.Docking;
import org.jdesktop.swingx.JXTreeTable;
import org.jdesktop.swingx.treetable.DefaultTreeTableModel;
import packets.PacketType;
import packets.TaskStateChange;
import taskglacier.MainFrame;

import javax.swing.*;
import javax.swing.table.DefaultTableModel;
import javax.swing.tree.TreePath;
import java.awt.*;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.io.IOException;

public class TasksLists extends JPanel implements Dockable, TaskModel.Listener {
    private final MainFrame mainFrame;
    private final String persistentID;
    private final String title;
    TasksTreeTableModel treeTableModel = new TasksTreeTableModel();
    JXTreeTable table = new JXTreeTable(treeTableModel);

    public TasksLists(MainFrame mainFrame, String persistentID, String title) {
        super(new BorderLayout());
        this.mainFrame = mainFrame;
        this.persistentID = persistentID;
        this.title = title;

        Docking.registerDockable(this);
        mainFrame.getTaskModel().addListener(this);

//        table.tableChanged();
        table.setShowsRootHandles(true);
        table.expandAll();

        JPopupMenu contextMenu = new JPopupMenu();
        JMenuItem start = new JMenuItem("Start");
        JMenuItem stop = new JMenuItem("Stop");
        JMenuItem finish = new JMenuItem("Finish");

        start.addActionListener(e -> {
            int selectedRow = table.getSelectedRow();

            if (selectedRow == -1) {
                return;
            }

            TreePath pathForRow = table.getPathForRow(selectedRow);



            TaskStateChange change = new TaskStateChange();
            change.packetType = PacketType.START_TASK;
            change.taskID = (int) treeTableModel.getValueAt(pathForRow.getLastPathComponent(), 0);
            mainFrame.getConnection().sendPacket(change);
        });
        stop.addActionListener(e -> {
            int selectedRow = table.getSelectedRow();

            if (selectedRow == -1) {
                return;
            }
            TreePath pathForRow = table.getPathForRow(selectedRow);

            TaskStateChange change = new TaskStateChange();
            change.packetType = PacketType.STOP_TASK;
            change.taskID = (int) treeTableModel.getValueAt(pathForRow.getLastPathComponent(), 0);
            mainFrame.getConnection().sendPacket(change);
        });
        finish.addActionListener(e -> {
            int selectedRow = table.getSelectedRow();

            if (selectedRow == -1) {
                return;
            }

            TreePath pathForRow = table.getPathForRow(selectedRow);

            TaskStateChange change = new TaskStateChange();
            change.packetType = PacketType.FINISH_TASK;
            change.taskID = (int) treeTableModel.getValueAt(pathForRow.getLastPathComponent(), 0);
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
//        treeTableModel.setRowCount(0);
    }

    @Override
    public void newTask(Task task) {
//        String name = mainFrame.getTaskModel().getTaskName(taskID);
//        treeTableModel.addRow(new Object[] { taskID, name });
//        treeTableModel.fireTableRowsInserted(treeTableModel.getRowCount() - 1, treeTableModel.getRowCount());


//        parentTask.addTask(task);
treeTableModel.addTask(task);


        table.expandAll();
    }

    @Override
    public void updatedTask(Task task) {
//        if (mainFrame.getTaskModel().getTaskState(taskID) == TaskState.FINISHED) {
//            for (int i = 0; i < treeTableModel.getRowCount(); i++) {
//                if ((int) treeTableModel.getValueAt(i, 0) == taskID) {
//                    treeTableModel.removeRow(i);
//                    break;
//                }
//            }
//        }
    }
}
