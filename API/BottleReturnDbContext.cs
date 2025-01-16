using Microsoft.EntityFrameworkCore;
using BottleReturnApi.Models;
using BottleReturnApi.Controllers;

namespace BottleReturnApi.Data
{
    public class BottleReturnDbContext(DbContextOptions<BottleReturnDbContext> options) : DbContext(options)
    {
        public DbSet<BarcodeValue> BarcodeValues { get; set; }
        public DbSet<LogEntry> Logs { get; set; }
        public DbSet<BottleReturnController.BottleReturnRecord> BottleReturns { get; set; }
        public DbSet<Scanner> Scanners { get; set; }
        public DbSet<Donateur> Donateurs { get; set; }

        protected override void OnModelCreating(ModelBuilder modelBuilder)
        {
            modelBuilder.HasDefaultSchema("dbo");
   
            modelBuilder.Entity<BarcodeValue>()
                .ToTable("waarde")
                .Property(b => b.bedrag)
                .HasPrecision(18, 2);

            modelBuilder.Entity<LogEntry>()
                .ToTable("logs");

            modelBuilder.Entity<Scanner>()
                .ToTable("scanner");

            modelBuilder.Entity<Donateur>()
                .ToTable("donateur")
                .Property(d => d.ID)
                .ValueGeneratedOnAdd();
        }
    }
}
