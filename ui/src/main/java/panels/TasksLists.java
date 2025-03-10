package panels;

import data.Task;
import data.TaskModel;
import data.TaskState;
import dialogs.AddModifyTask;
import dialogs.EditLabels;
import dialogs.RenameTask;
import io.github.andrewauclair.moderndocking.Dockable;
import io.github.andrewauclair.moderndocking.DockingProperty;
import io.github.andrewauclair.moderndocking.DockingRegion;
import io.github.andrewauclair.moderndocking.app.Docking;
import org.jdesktop.swingx.JXTreeTable;
import packets.PacketType;
import packets.RequestID;
import packets.TaskStateChange;
import packets.UpdateTask;
import taskglacier.MainFrame;

import javax.swing.*;
import javax.swing.tree.DefaultTreeCellRenderer;
import javax.swing.tree.TreePath;
import java.awt.*;
import java.awt.datatransfer.StringSelection;
import java.awt.datatransfer.Transferable;
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

    private boolean allTasks = false;

    private TaskInfoSubPanel infoSubPanel = new TaskInfoSubPanel();

    // TODO right now Modern Docking uses the persistentID as both parameters. That's not ideal.
    public TasksLists(String persistentID, String title) {
        super(new BorderLayout());

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
        persistentID = "task-list-" + task.id;
        title = "Tasks (" + task.name + ")";

        Docking.registerDockable(this);
        mainFrame.getTaskModel().addListener(this);

        treeTableModel = new TasksTreeTableModel(new ParentTaskTreeTableNode());
        table = new JXTreeTable(treeTableModel);

        treeTableModel.addTask(task);

        table.setShowsRootHandles(true);
        table.setRootVisible(false);

        table.expandAll();

        configure();
    }

    public TasksLists(MainFrame mainFrame, String persistentID, String title) {
        super(new BorderLayout());

        allTasks = true;

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

            if (task != null) {
                table.setRootVisible(true);
                treeTableModel.setRoot(new TaskTreeTableNode(null, task));
                addTasks(task);
            }

            table.expandAll();
        }
    }

    private void addTasks(Task task) {
        for (Task child : task.children) {
            treeTableModel.addTask(child);
        }
    }

    @Override
    public boolean isClosable() {
        return taskID != 0;
    }

    class TaskTransferHandler extends TransferHandler {
        private int[] rows;

        @Override
        public int getSourceActions(JComponent c) {
            return MOVE;
        }

        @Override
        protected Transferable createTransferable(JComponent c) {
            rows = table.getSelectedRows();

            int parent = -1;

            // all the rows must have the same parent
            for (int row : rows) {
                TreePath pathForRow = table.getPathForRow(row);
                TaskTreeTableNode node = (TaskTreeTableNode) pathForRow.getLastPathComponent();
                Task task = (Task) node.getUserObject();

                if (parent == -1) {
                    parent = task.parentID;
                }
                else if (task.parentID != parent) {
                    return null;
                }
            }

            return new StringSelection("");
        }

        @Override
        public boolean canImport(TransferSupport support) {
            return true;
        }

        @Override
        public boolean importData(TransferSupport support) {
            // send to server
            for (int row : rows) {
                TreePath pathForRow = table.getPathForRow(row);
                TaskTreeTableNode node = (TaskTreeTableNode) pathForRow.getLastPathComponent();
                Task task = (Task) node.getUserObject();

                JTable.DropLocation dl = (JTable.DropLocation) support.getDropLocation();

                TaskTreeTableNode parentNode = (TaskTreeTableNode) table.getPathForRow(dl.getRow()).getLastPathComponent();
                Task parentTask = (Task) parentNode.getUserObject();

                // move task: request id, task id, new parent id
                UpdateTask update = new UpdateTask(RequestID.nextRequestID(), task.id, parentTask.id, task.name);
                mainFrame.getConnection().sendPacket(update);
            }
            return true;
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
        table.setDragEnabled(true);
        table.setDropMode(DropMode.ON_OR_INSERT_ROWS);

        table.setTransferHandler(new TaskTransferHandler());

        JMenuItem add = new JMenuItem("Add Task...");
        JMenuItem addSubTask = new JMenuItem("Add Sub-Task...");
        JMenuItem rename = new JMenuItem("Rename...");
        JMenuItem editLabels = new JMenuItem("Edit Labels...");
        JMenuItem editTime = new JMenuItem("Edit Time...");
        JMenuItem start = new JMenuItem("Start");
        JMenuItem startStopActive = new JMenuItem("Start (Stop Active)");
        JMenuItem startFinishActive = new JMenuItem("Start (Finish Active)");
        JMenuItem stop = new JMenuItem("Stop");
        JMenuItem finish = new JMenuItem("Finish");
        JMenuItem openInNewWindow = new JMenuItem("Open in New List");

        start.addActionListener(e -> changeTaskState(PacketType.START_TASK));
        startStopActive.addActionListener(e -> changeTaskState(PacketType.START_TASK));
        startFinishActive.addActionListener(e -> {
            finishActiveTask();
            changeTaskState(PacketType.START_TASK);
        });
        stop.addActionListener(e -> changeTaskState(PacketType.STOP_TASK));
        finish.addActionListener(e -> changeTaskState(PacketType.FINISH_TASK));

        add.addActionListener(e -> new AddModifyTask(mainFrame, 0, false).setVisible(true));

        addSubTask.addActionListener(e -> {
            int selectedRow = table.getSelectedRow();

            if (selectedRow == -1) {
                return;
            }

            TreePath pathForRow = table.getPathForRow(selectedRow);
            TaskTreeTableNode node = (TaskTreeTableNode) pathForRow.getLastPathComponent();
            Task task = (Task) node.getUserObject();

            new AddModifyTask(mainFrame, task.id, false).setVisible(true);
        });

        rename.addActionListener(e -> {
            int selectedRow = table.getSelectedRow();

            if (selectedRow == -1) {
                return;
            }

            TreePath pathForRow = table.getPathForRow(selectedRow);
            TaskTreeTableNode node = (TaskTreeTableNode) pathForRow.getLastPathComponent();
            Task task = (Task) node.getUserObject();

            RenameTask dialog = new RenameTask(mainFrame, task.id, task.parentID, task.name);

            dialog.setVisible(true);
        });

        editLabels.addActionListener(e -> {
            int selectedRow = table.getSelectedRow();

            if (selectedRow == -1) {
                return;
            }

            TreePath pathForRow = table.getPathForRow(selectedRow);
            TaskTreeTableNode node = (TaskTreeTableNode) pathForRow.getLastPathComponent();
            Task task = (Task) node.getUserObject();

            EditLabels dialog = new EditLabels(task);

            dialog.setVisible(true);
        });

        openInNewWindow.addActionListener(e -> {
            int selectedRow = table.getSelectedRow();

            if (selectedRow == -1) {
                return;
            }

            TreePath pathForRow = table.getPathForRow(selectedRow);
            TaskTreeTableNode node = (TaskTreeTableNode) pathForRow.getLastPathComponent();
            Task task = (Task) node.getUserObject();

            if (!Docking.isDockableRegistered("task-list-" + task.id)) {
                TasksLists newList = new TasksLists(mainFrame, task);
                Docking.dock(newList, TasksLists.this, DockingRegion.CENTER);
            }
            else {
                Docking.display("task-list-" + task.id);
            }
        });

        table.addMouseListener(new MouseAdapter() {
            @Override
            public void mouseClicked(MouseEvent e) {
                if (SwingUtilities.isRightMouseButton(e)) {
                    int selectedRow = table.getSelectedRow();

                    JPopupMenu contextMenu = new JPopupMenu();

                    if (selectedRow == -1) {
                        contextMenu.add(add);
                        contextMenu.show(table, e.getX(), e.getY());
                        return;
                    }

                    if (mainFrame.getTaskModel().getActiveTaskID().isPresent() && !mainFrame.getTaskModel().taskHasNonFinishedChildren(mainFrame.getTaskModel().getActiveTaskID().get())) {
                        contextMenu.add(startStopActive);
                        contextMenu.add(startFinishActive);
                    }
                    else {
                        contextMenu.add(start);
                    }

                    contextMenu.add(stop);
                    contextMenu.add(finish);
                    contextMenu.addSeparator();
                    contextMenu.add(addSubTask);
                    contextMenu.addSeparator();
                    contextMenu.add(rename);
                    contextMenu.add(editLabels);
                    contextMenu.add(editTime);

                    TreePath pathForRow = table.getPathForRow(selectedRow);
                    TaskTreeTableNode node = (TaskTreeTableNode) pathForRow.getLastPathComponent();
                    Task task = (Task) node.getUserObject();

                    startStopActive.setEnabled(task.state == TaskState.INACTIVE);
                    startFinishActive.setEnabled(task.state == TaskState.INACTIVE);
                    stop.setEnabled(task.state == TaskState.ACTIVE);
                    finish.setEnabled(task.state != TaskState.FINISHED);

                    // task has subtasks, allow an option to open it in a new panel
                    if (!node.isLeaf()) {
                        contextMenu.addSeparator();
                        contextMenu.add(openInNewWindow);
                    }

                    contextMenu.show(table, e.getX(), e.getY());
                }
                else if (e.getClickCount() == 2 && SwingUtilities.isLeftMouseButton(e)) {
                    start.doClick();
                }
            }
        });

        table.addTreeSelectionListener(e -> {
            if (e.getNewLeadSelectionPath() == null) {
                infoSubPanel.displayForTask(null);
                return;
            }
            TaskTreeTableNode lastPathComponent = (TaskTreeTableNode) e.getNewLeadSelectionPath().getLastPathComponent();

            infoSubPanel.displayForTask((Task) lastPathComponent.getUserObject());
        });

        JSplitPane split = new JSplitPane();

        split.setLeftComponent(new JScrollPane(table));
        split.setRightComponent(infoSubPanel);

        add(split);
    }

    private void finishActiveTask() {
        TaskStateChange change = new TaskStateChange();
        change.packetType = PacketType.FINISH_TASK;
        change.taskID = mainFrame.getTaskModel().getActiveTaskID().get();
        mainFrame.getConnection().sendPacket(change);
    }

    private void changeTaskState(PacketType type) {
        int selectedRow = table.getSelectedRow();

        if (selectedRow == -1) {
            return;
        }

        TreePath pathForRow = table.getPathForRow(selectedRow);
        TaskTreeTableNode node = (TaskTreeTableNode) pathForRow.getLastPathComponent();
        Task task = (Task) node.getUserObject();

        TaskStateChange change = new TaskStateChange();
        change.packetType = type;
        change.taskID = task.id;
        mainFrame.getConnection().sendPacket(change);
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
    public void updatedTask(Task task, boolean parentChanged) {
        treeTableModel.updateTask(task, parentChanged);
    }
}
