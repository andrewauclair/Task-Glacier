package panels;

import data.Task;
import data.TaskModel;
import io.github.andrewauclair.moderndocking.Dockable;
import io.github.andrewauclair.moderndocking.DockingProperty;
import io.github.andrewauclair.moderndocking.DockingRegion;
import io.github.andrewauclair.moderndocking.app.Docking;
import org.jdesktop.swingx.JXTreeTable;
import packets.PacketType;
import packets.TaskStateChange;
import taskglacier.MainFrame;

import javax.swing.*;
import javax.swing.tree.DefaultTreeCellRenderer;
import javax.swing.tree.TreePath;
import java.awt.*;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.util.Objects;

public class TasksLists extends JPanel implements Dockable, TaskModel.Listener {
    private final MainFrame mainFrame;
    private final String persistentID;
    private final String title;
    private final TasksTreeTableModel treeTableModel;
    private final JXTreeTable table;

    @DockingProperty(name = "taskID")
    private int taskID = 0;

    public TasksLists(String persistentID, String title) {
        this.persistentID = persistentID;
        this.title = "Tasks";
        mainFrame = MainFrame.mainFrame;

        Docking.registerDockable(this);
        mainFrame.getTaskModel().addListener(this);

        treeTableModel = new TasksTreeTableModel(new ParentTaskTreeTableNode());
        table = new JXTreeTable(treeTableModel);
        table.setShowsRootHandles(true);
        table.setRootVisible(false);

        table.expandAll();

        configure();
    }

    public TasksLists(MainFrame mainFrame, Task task) {
        super(new BorderLayout());

        taskID = task.id;
        this.mainFrame = mainFrame;
        persistentID = task.name;
        title = "Tasks (" + task.name + ")";

        Docking.registerDockable(this);
        mainFrame.getTaskModel().addListener(this);

        treeTableModel = new TasksTreeTableModel(new ParentTaskTreeTableNode());
        table = new JXTreeTable(treeTableModel);

        treeTableModel.addTask(task);

        table.setShowsRootHandles(true);
        table.setRootVisible(true);

        table.expandAll();

        configure();
    }

    public TasksLists(MainFrame mainFrame, String persistentID, String title) {
        super(new BorderLayout());

        this.mainFrame = mainFrame;
        this.persistentID = persistentID;
        this.title = title;

        Docking.registerDockable(this);
        mainFrame.getTaskModel().addListener(this);

        treeTableModel = new TasksTreeTableModel(new ParentTaskTreeTableNode());
        table = new JXTreeTable(treeTableModel);
        table.setShowsRootHandles(true);
        table.setRootVisible(false);

        table.expandAll();

        configure();
    }

    @Override
    public void updateProperties() {
        if (taskID == 0) {
            table.expandAll();
        }
        else {
            Task task = mainFrame.getTaskModel().getTask(taskID);

            treeTableModel.addTask(task);

            table.expandAll();
        }
    }

    private void configure() {
        DefaultTreeCellRenderer  treeCellRenderer = new DefaultTreeCellRenderer () {
            @Override
            public Component getTreeCellRendererComponent(JTree tree, Object value, boolean selected, boolean expanded, boolean leaf, int row, boolean hasFocus) {
                Component c = super.getTreeCellRendererComponent(tree, value, selected, expanded, leaf, row, hasFocus);

                TaskTreeTableNode node = (TaskTreeTableNode) value;

                Task task = (Task) node.getUserObject();

                if (task != null) {
                    setText(task.name);
                }
                else {
                    setText("Empty?");
                }

                if (node.isActiveTask()) {
                    setIcon(new ImageIcon(Objects.requireNonNull(getClass().getResource("/active.png"))));
                }
                else if (node.hasActiveChildTask()) {
                    setIcon(new ImageIcon(Objects.requireNonNull(getClass().getResource("/activeChild.png"))));
                }

                return c;
            }
        };
        table.setTreeCellRenderer(treeCellRenderer);

        JMenuItem start = new JMenuItem("Start");
        JMenuItem stop = new JMenuItem("Stop");
        JMenuItem finish = new JMenuItem("Finish");
        JMenuItem openInNewWindow = new JMenuItem("Open in New List");

        start.addActionListener(e -> {
            int selectedRow = table.getSelectedRow();

            if (selectedRow == -1) {
                return;
            }

            TreePath pathForRow = table.getPathForRow(selectedRow);
            TaskTreeTableNode node = (TaskTreeTableNode) pathForRow.getLastPathComponent();
            Task task = (Task) node.getUserObject();

            TaskStateChange change = new TaskStateChange();
            change.packetType = PacketType.START_TASK;
            change.taskID = task.id;
            mainFrame.getConnection().sendPacket(change);
        });
        stop.addActionListener(e -> {
            int selectedRow = table.getSelectedRow();

            if (selectedRow == -1) {
                return;
            }

            TreePath pathForRow = table.getPathForRow(selectedRow);
            TaskTreeTableNode node = (TaskTreeTableNode) pathForRow.getLastPathComponent();
            Task task = (Task) node.getUserObject();

            TaskStateChange change = new TaskStateChange();
            change.packetType = PacketType.STOP_TASK;
            change.taskID = task.id;
            mainFrame.getConnection().sendPacket(change);
        });
        finish.addActionListener(e -> {
            int selectedRow = table.getSelectedRow();

            if (selectedRow == -1) {
                return;
            }

            TreePath pathForRow = table.getPathForRow(selectedRow);
            TaskTreeTableNode node = (TaskTreeTableNode) pathForRow.getLastPathComponent();
            Task task = (Task) node.getUserObject();

            TaskStateChange change = new TaskStateChange();
            change.packetType = PacketType.FINISH_TASK;
            change.taskID = task.id;
            mainFrame.getConnection().sendPacket(change);
        });

        openInNewWindow.addActionListener(e -> {
            int selectedRow = table.getSelectedRow();

            if (selectedRow == -1) {
                return;
            }

            TreePath pathForRow = table.getPathForRow(selectedRow);
            TaskTreeTableNode node = (TaskTreeTableNode) pathForRow.getLastPathComponent();
            Task task = (Task) node.getUserObject();

            TasksLists newList = new TasksLists(mainFrame, task);
            Docking.dock(newList, TasksLists.this, DockingRegion.CENTER);
        });

        table.addMouseListener(new MouseAdapter() {
            @Override
            public void mouseClicked(MouseEvent e) {
                if (SwingUtilities.isRightMouseButton(e)) {
                    int selectedRow = table.getSelectedRow();

                    JPopupMenu contextMenu = new JPopupMenu();

                    if (selectedRow == -1) {
                        // TODO allow an add task option
                        return;
                    }

                    contextMenu.add(start);
                    contextMenu.add(stop);
                    contextMenu.add(finish);

                    TreePath pathForRow = table.getPathForRow(selectedRow);
                    TaskTreeTableNode node = (TaskTreeTableNode) pathForRow.getLastPathComponent();

                    // task has subtasks, allow an option to open it in a new panel
                    if (!node.isLeaf()) {
                        contextMenu.add(openInNewWindow);
                    }

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
    public void newTask(Task task) {
        treeTableModel.addTask(task);
    }

    @Override
    public void updatedTask(Task task) {
        treeTableModel.updateTask(task);
    }
}
