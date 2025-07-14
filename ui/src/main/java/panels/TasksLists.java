package panels;

import data.Task;
import data.TaskModel;
import data.TaskState;
import dialogs.AddModifyTask;
import dialogs.TaskConfig;
import io.github.andrewauclair.moderndocking.Dockable;
import io.github.andrewauclair.moderndocking.DockingProperty;
import io.github.andrewauclair.moderndocking.DockingRegion;
import io.github.andrewauclair.moderndocking.DynamicDockableParameters;
import io.github.andrewauclair.moderndocking.app.Docking;
import org.jdesktop.swingx.JXTreeTable;
import packets.PacketType;
import packets.RequestID;
import packets.TaskStateChange;
import packets.UpdateTask;
import taskglacier.MainFrame;

import javax.swing.*;
import javax.swing.table.TableCellRenderer;
import javax.swing.table.TableColumnModel;
import javax.swing.tree.DefaultTreeCellRenderer;
import javax.swing.tree.TreePath;
import java.awt.*;
import java.awt.datatransfer.StringSelection;
import java.awt.datatransfer.Transferable;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.util.Objects;

public class TasksLists extends JPanel implements Dockable, TaskModel.Listener {
    @DockingProperty(name = "taskID", required = true)
    private int taskID = 0;

    @DockingProperty(name = "allTasks", required = true)
    private boolean allTasks = false;

    private String persistentID;
    private String titleText;
    private String tabText;

    private MainFrame mainFrame;
    private final TasksTreeTableModel treeTableModel;
    private final JXTreeTable table;

    private TaskInfoSubPanel infoSubPanel = new TaskInfoSubPanel();

    public TasksLists(MainFrame mainFrame) {
        super(new BorderLayout());

        allTasks = true;

        this.mainFrame = mainFrame;
        this.persistentID = "tasks";
        this.titleText = "Tasks";
        this.tabText = "Tasks";

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
        titleText = "Tasks (" + task.name + ")";
        tabText = "Tasks (" + task.name + ")";

        Docking.registerDockable(this);
        mainFrame.getTaskModel().addListener(this);

        treeTableModel = new TasksTreeTableModel(new ParentTaskTreeTableNode(task));
        table = new JXTreeTable(treeTableModel);

        addTasks(task);

        table.setShowsRootHandles(true);
        table.setRootVisible(false);

        table.expandAll();

        configure();
    }

    public TasksLists(DynamicDockableParameters parameters) {
        super(new BorderLayout());

        this.persistentID = parameters.getPersistentID();
        this.titleText = parameters.getTitleText();
        this.tabText = parameters.getTabText();

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

    @Override
    public void updateProperties() {
        mainFrame = MainFrame.mainFrame;

        if (allTasks) {
            table.expandAll();
        }
        else {
            Task task = mainFrame.getTaskModel().getTask(taskID);

            if (task != null) {
                table.setRootVisible(false);
                treeTableModel.setRoot(new ParentTaskTreeTableNode(task));
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
        table.setAutoResizeMode(JTable.AUTO_RESIZE_LAST_COLUMN);

        table.setTransferHandler(new TaskTransferHandler());

//        TableRowSorter<TableModel> sorter = new TableRowSorter<>(table.getModel());
//        table.setRowSorter(sorter);
//
//        List<RowSorter.SortKey> sortKeys = new ArrayList<>(25);
//        sortKeys.add(new RowSorter.SortKey(1, SortOrder.ASCENDING));
//        sorter.setSortKeys(sortKeys);

        JMenuItem add = new JMenuItem("Add Task...");
        JMenuItem addSubTask = new JMenuItem("Add Sub-Task...");
        JMenuItem start = new JMenuItem("Start");
        JMenuItem startStopActive = new JMenuItem("Start (Stop Active)");
        JMenuItem startFinishActive = new JMenuItem("Start (Finish Active)");
        JMenuItem stop = new JMenuItem("Stop");
        JMenuItem finish = new JMenuItem("Finish");
        JMenuItem openInNewWindow = new JMenuItem("Open in New List");
        JMenuItem config = new JMenuItem("Configure...");

        config.addActionListener(e -> {
            int selectedRow = table.getSelectedRow();

            if (selectedRow == -1) {
                return;
            }

            TreePath pathForRow = table.getPathForRow(selectedRow);
            TaskTreeTableNode node = (TaskTreeTableNode) pathForRow.getLastPathComponent();
            Task task = (Task) node.getUserObject();

            TaskConfig dialog = new TaskConfig(mainFrame, mainFrame, task);
            dialog.setVisible(true);
        });

        start.addActionListener(e -> changeTaskState(PacketType.START_TASK));
        startStopActive.addActionListener(e -> changeTaskState(PacketType.START_TASK));
        startFinishActive.addActionListener(e -> {
            finishActiveTask();
            changeTaskState(PacketType.START_TASK);
        });
        stop.addActionListener(e -> changeTaskState(PacketType.STOP_TASK));
        finish.addActionListener(e -> changeTaskState(PacketType.FINISH_TASK));

        add.addActionListener(e -> new AddModifyTask(mainFrame, mainFrame, 0, false).setVisible(true));

        addSubTask.addActionListener(e -> {
            int selectedRow = table.getSelectedRow();

            if (selectedRow == -1) {
                return;
            }

            TreePath pathForRow = table.getPathForRow(selectedRow);
            TaskTreeTableNode node = (TaskTreeTableNode) pathForRow.getLastPathComponent();
            Task task = (Task) node.getUserObject();

            new AddModifyTask(mainFrame, mainFrame, task.id, false).setVisible(true);
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

                    contextMenu.add(config);

                    if (mainFrame.getTaskModel().getActiveTaskID().isPresent() &&
                            !mainFrame.getTaskModel().taskHasNonFinishedChildren(mainFrame.getTaskModel().getActiveTaskID().get()) &&
                            !mainFrame.getTaskModel().getTask(mainFrame.getTaskModel().getActiveTaskID().get()).locked) {
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

                    TreePath pathForRow = table.getPathForRow(selectedRow);
                    TaskTreeTableNode node = (TaskTreeTableNode) pathForRow.getLastPathComponent();
                    Task task = (Task) node.getUserObject();

                    startStopActive.setEnabled(task.state == TaskState.PENDING);
                    startFinishActive.setEnabled(task.state == TaskState.PENDING);
                    stop.setEnabled(task.state == TaskState.ACTIVE);
                    finish.setEnabled(task.state != TaskState.FINISHED && !mainFrame.getTaskModel().taskHasNonFinishedChildren(task.id) && !task.locked && !task.serverControlled);

                    // task has subtasks, allow an option to open it in a new panel
                    if (!node.isLeaf()) {
                        contextMenu.addSeparator();
                        contextMenu.add(openInNewWindow);
                    }

                    contextMenu.show(table, e.getX(), e.getY());
                }
                else if (e.getClickCount() == 2 && SwingUtilities.isLeftMouseButton(e)) {
                    config.doClick();
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

//        JSplitPane split = new JSplitPane();
//
//        split.setLeftComponent(new JScrollPane(table));
//        split.setRightComponent(infoSubPanel);
//
        add(new JScrollPane(table));
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
    public String getTitleText() {
        return titleText;
    }

    @Override
    public String getTabText() {
        return tabText;
    }

    @Override
    public boolean isWrappableInScrollpane() {
        return false;
    }

    private void resizeColumnWidth() {
        final TableColumnModel columnModel = table.getColumnModel();
        for (int column = 0; column < table.getColumnCount(); column++) {
            // Account for header size
            double width = table.getTableHeader().getHeaderRect(column).getWidth();
            for (int row = 0; row < table.getRowCount(); row++) {
                TableCellRenderer renderer = table.getCellRenderer(row, column);
                Component comp = table.prepareRenderer(renderer, row, column);
                width = Math.max(comp.getPreferredSize().width + 1, width);
            }

            columnModel.getColumn(column).setPreferredWidth((int) width);
        }
    }

    @Override
    public void newTask(Task task) {
        treeTableModel.addTask(task);

        resizeColumnWidth();
    }

    @Override
    public void updatedTask(Task task, boolean parentChanged) {
        treeTableModel.updateTask(task, parentChanged);

        resizeColumnWidth();
    }
}
